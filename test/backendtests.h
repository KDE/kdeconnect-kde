/*
 * Copyright 2013 <copyright holder> <email>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef BACKENDTESTS_H
#define BACKENDTESTS_H

#include <QObject>

class BackendTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void dummyTest();
    void networkPackageTest();

    void cleanupTestCase();

    void init();
    void cleanup();

};

#endif // BACKENDTESTS_H
