#pragma once

// A throwaway mongod for integration tests: bound to 127.0.0.1 on a free port, data in a
// temporary directory that is deleted afterwards, shut down when the test case finishes.
//
// mongod is looked up in $ROBO_TEST_MONGOD, then on PATH, then in Homebrew's bin directories.
// When it is not installed, available() is false and integration tests skip themselves.

#include <cstdio>
#include <memory>
#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QThread>

#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include "robomongo/core/mongodb/modern/ModernDriverRuntime.h"

namespace Robomongo
{
    class TestMongoServer
    {
    public:
        TestMongoServer()
        {
            QString const executable = findMongod();
            if (executable.isEmpty() || !_dataDir.isValid())
                return;

            _port = freePort();

            // mongod runs under a small shell watchdog: it stops mongod when asked to terminate,
            // and also when this test process goes away (e.g. a crashing test), so no server is
            // ever left running.
            QString const watchdog =
                "\"$0\" \"$@\" & m=$!; "
                "trap 'kill $m 2>/dev/null; wait $m; exit 0' TERM; "
                "while kill -0 " + QString::number(QCoreApplication::applicationPid()) + " 2>/dev/null"
                " && kill -0 $m 2>/dev/null; do sleep 1; done; "
                "kill $m 2>/dev/null; wait $m";
            _process.reset(new QProcess());
            _process->setProgram("/bin/sh");
            _process->setArguments({ "-c", watchdog, executable,
                                     "--dbpath", _dataDir.path(), "--port", QString::number(_port),
                                     "--bind_ip", "127.0.0.1", "--nounixsocket", "--quiet",
                                     "--logpath", _dataDir.path() + "/mongod.log" });
            _process->start();
            if (!_process->waitForStarted(10000))
                return;

            _available = waitUntilReady();
        }

        ~TestMongoServer()
        {
            if (!_process)
                return;
            _process->terminate();  // watchdog shuts mongod down cleanly
            if (!_process->waitForFinished(20000)) {
                _process->kill();
                _process->waitForFinished(5000);
            }
        }

        bool available() const { return _available; }
        int port() const { return _port; }
        std::string uri() const { return "mongodb://127.0.0.1:" + std::to_string(_port); }

        mongocxx::client client() const { return mongocxx::client(mongocxx::uri(uri())); }

        static QString findMongod()
        {
            QString const fromEnv = qEnvironmentVariable("ROBO_TEST_MONGOD");
            if (!fromEnv.isEmpty())
                return fromEnv;
            QString const onPath = QStandardPaths::findExecutable("mongod");
            if (!onPath.isEmpty())
                return onPath;
            return QStandardPaths::findExecutable("mongod", { "/opt/homebrew/bin", "/usr/local/bin" });
        }

    private:
        static int freePort()
        {
            QTcpServer probe;
            probe.listen(QHostAddress::LocalHost, 0);
            return probe.serverPort();
        }

        bool waitUntilReady() const
        {
            modernDriverRuntime();
            for (int attempt = 0; attempt < 100; ++attempt) {
                if (_process->state() != QProcess::Running)
                    return false;
                try {
                    mongocxx::client c(mongocxx::uri(uri() + "/?serverSelectionTimeoutMS=200"));
                    c["admin"].run_command(bsoncxx::builder::basic::make_document(
                        bsoncxx::builder::basic::kvp("ping", 1)));
                    return true;
                } catch (const std::exception &) {
                    QThread::msleep(200);
                }
            }
            return false;
        }

        QTemporaryDir _dataDir;
        std::unique_ptr<QProcess> _process;
        int _port = 0;
        bool _available = false;
    };
}

// Skips the rest of an integration test when no mongod could be started
#define ROBO_REQUIRE_MONGOD(server)                                                         \
    if (!(server) || !(server)->available()) {                                             \
        std::printf("    [ skipped ] mongod not available (set ROBO_TEST_MONGOD)\n");     \
        return;                                                                             \
    }
