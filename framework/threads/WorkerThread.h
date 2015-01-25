#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H
#include "common.h"
#include "WorkerPackage.h"
#include "MutexLock.h"
namespace RackoonIO {

class WorkerThread : public MutexLock {
	void process();
	std::thread *worker;
	bool busy;
	bool running;
	std::unique_ptr<WorkerPackage> current;

	std::mutex pkg_lock;
public:
	WorkerThread(bool = false);
	void start();
	void stop();
	bool isBusy();

	bool assignPackage(std::unique_ptr<WorkerPackage>);
};

}
#endif
