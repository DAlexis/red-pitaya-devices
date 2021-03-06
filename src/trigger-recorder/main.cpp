/* Red Pitaya C API example Acquiring a signal from a buffer  
 * This application acquires a signal on a specific channel */

#include "trigger-recorder.hpp"

#include <iostream>
#include <memory>
#include <signal.h>

using namespace std;

unique_ptr<TriggerRecorder> t;

void signalHandler(int signum)
{
	switch(signum)
	{
	case SIGINT:
		cout << "Interruped by user" << endl;
		t->stop();
		break;
	case SIGTERM:
		cout << "Interruped by SIGTERM" << endl;
		t->stop();
		break;
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	t.reset(new TriggerRecorder);
	if (t->parseParameters(argc, argv))
		t->run();

	return 0;
}
        
