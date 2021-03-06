// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'package:expect/expect.dart';

import '../github_services.dart';
import '../github_mock.dart';

void main() {
  var mock = new GithubMock()..spawn();
  var server = new Server(mock.host, mock.port);
  testError(server);
  testUser(server);
  testRepository(server);
  testCommits(server);
  mock.close();
}

void testUser(Server server) {
  var user = server.getUser('dart-lang');
  Expect.stringEquals('dart-lang', user['login']);
}

void testRepository(Server server) {
  var user = server.getUser('dart-lang');
  var repo = user.getRepository('fletch');
  Expect.stringEquals('dart-lang/fletch', repo['full_name']);
}

void testError(Server server) {
  var user = server.getUser('dart-lang-no-such-user');
  Expect.throws(() { user['login']; });
}

void testCommits(Server server) {
  var user = server.getUser('dart-lang');
  var repo = user.getRepository('fletch');
  var commit = repo.getCommitAt(0);
  Expect.stringEquals('Ian Zerny', commit['commit']['author']['name']);
  commit = repo.getCommitAt(60);
  Expect.stringEquals("Anders Johnsen", commit['commit']['author']['name']);
}
