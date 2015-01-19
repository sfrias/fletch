// Copyright (c) 2014, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "src/vm/stack_walker.h"

#include "src/shared/bytecodes.h"
#include "src/shared/utils.h"

#include "src/vm/program.h"
#include "src/vm/session.h"

namespace fletch {

bool StackWalker::MoveNext() {
  int bcp_offset = stack_->top() + stack_offset_;
  uint8* bcp = reinterpret_cast<uint8*>(stack_->get(bcp_offset));

  // At bottom.
  if (bcp == NULL) {
    function_ = NULL;
    return_address_ = NULL;
    frame_size_ = -1;
    frame_ranges_offset_ = -1;
    return false;
  }

  Program* program = process_->program();
  ASSERT(program->heap()->space()->Includes(reinterpret_cast<uword>(bcp)));

  return_address_ = bcp;
  bool first = function_ == NULL;
  function_ = Function::FromBytecodePointer(bcp, &frame_ranges_offset_);
  frame_size_ = ComputeStackOffset(function_, bcp, program, first);
  stack_offset_ -= (frame_size_ + 1);

  return true;
}

int StackWalker::CookFrame() {
  uint8_t* start = function_->bytecode_address_for(0);
  stack_->set(stack_->top() + stack_offset_ + frame_size_ + 1, function_);
  return return_address_ - start;
}

void StackWalker::UncookFrame(int delta) {
  Object* current = stack_->get(stack_->top() + stack_offset_);
  if (current == NULL) return;
  Function* function = Function::cast(current);
  uint8_t* start = function->bytecode_address_for(0);
  Object* bcp = reinterpret_cast<Object*>(start + delta);
  stack_->set(stack_->top() + stack_offset_, bcp);
}

static int StackDiff(uint8** bcp,
                     uint8* end_bcp,
                     Program* program,
                     int current_stack_offset) {
  int stack_diff = kVarDiff;

  Opcode opcode = static_cast<Opcode>(**bcp);
  switch (opcode) {
    case kInvokeMethod: {
      int arity = Utils::ReadInt32(*bcp + 1) & 0xFF;
      stack_diff = -arity;
      break;
    }

    case kInvokeStatic:
    case kInvokeFactory: {
      int method = Utils::ReadInt32(*bcp + 1);
      Function* function = program->static_method_at(method);
      stack_diff = 1 - function->arity();
      break;
    }

    case kInvokeStaticUnfold:
    case kInvokeFactoryUnfold: {
      Function* function = Function::cast(Function::ConstantForBytecode(*bcp));
      stack_diff = 1 - function->arity();
      break;
    }

    case kBranchLong:
    case kBranchIfTrueLong:
    case kBranchIfFalseLong: {
      int delta = Utils::ReadInt32(*bcp + 1);
      stack_diff = Bytecode::StackDiff(opcode);
      if (*bcp + delta <= end_bcp) {
        *bcp += delta;
        // Return as we have moved bcp with a custom delta.
        return stack_diff;
      }
      break;
    }

    case kSubroutineCall: {
      int delta = Utils::ReadInt32(*bcp + 1);
      if (*bcp + delta <= end_bcp) {
        *bcp += delta;
        return 1;
      }
      stack_diff = 0;
      break;
    }

    case kAllocate: {
      int class_id = Utils::ReadInt32(*bcp + 1);
      Class* klass = program->class_at(class_id);
      int fields = klass->NumberOfInstanceFields();
      stack_diff = 1 - fields;
      break;
    }

    case kAllocateUnfold: {
      Class* klass = Class::cast(Function::ConstantForBytecode(*bcp));
      int fields = klass->NumberOfInstanceFields();
      stack_diff = 1 - fields;
      break;
    }

    case kFrameSize: {
      stack_diff = (*bcp)[1] - current_stack_offset;
      break;
    }

    default:
      stack_diff = Bytecode::StackDiff(opcode);
      break;
  }
  ASSERT(stack_diff != kVarDiff);
  *bcp += Bytecode::Size(opcode);
  return stack_diff;
}

int StackWalker::ComputeStackOffset(Function* function,
                                    uint8* end_bcp,
                                    Program* program,
                                    bool include_last) {
  int stack_offset = 0;
  uint8* bcp = function->bytecode_address_for(0);
  int next_diff = 0;
  while (bcp != end_bcp) {
    ASSERT(bcp < end_bcp);
    stack_offset += next_diff;
    next_diff = StackDiff(&bcp, end_bcp, program, stack_offset);
  }
  ASSERT(bcp == end_bcp);
  if (include_last) stack_offset += next_diff;
  ASSERT(stack_offset >= 0);
  return stack_offset;
}

uint8* StackWalker::ComputeCatchBlock(Process* process, int* stack_delta) {
  int delta = 0;
  StackWalker walker(process, process->stack());
  while (walker.MoveNext()) {
    Function* function = walker.function();
    delta += 1 + walker.frame_size();
    int offset = walker.frame_ranges_offset();
    uint8* range_bcp = function->bytecode_address_for(offset);
    int count = Utils::ReadInt32(range_bcp);
    range_bcp += 4;
    for (int i = 0; i < count; i++) {
      int start = Utils::ReadInt32(range_bcp);
      uint8* start_address = function->bytecode_address_for(start);
      range_bcp += 4;
      int end = Utils::ReadInt32(range_bcp);
      uint8* end_address = function->bytecode_address_for(end);
      range_bcp += 4;
      uint8* return_address = walker.return_address();
      if (start_address < return_address && end_address > return_address) {
        // The first hit is the one we use (due to the order they are
        // emitted).
        delta -= StackWalker::ComputeStackOffset(function,
                                                 end_address,
                                                 process->program(),
                                                 true);
        *stack_delta = delta;
        return end_address;
      }
    }
  }
  return NULL;
}

int StackWalker::ComputeStackTrace(Process* process, Session* session) {
  int frames = 0;
  StackWalker walker(process, process->stack());
  while (walker.MoveNext()) {
    Function* function = walker.function();
    uint8* start_bcp = function->bytecode_address_for(0);
    int bytecode_offset = walker.return_address() - start_bcp;
    // The first byte-code offset is not a return address but the offset for
    // the throw bytecode. Make it look like a return address by adding
    // one to the byte-code offset.
    if (frames == 0) ++bytecode_offset;
    session->PushNewInteger(bytecode_offset);
    session->PushFunction(function);
    ++frames;
  }
  return frames;
}

}  // namespace fletch