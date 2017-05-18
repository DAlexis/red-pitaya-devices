/* Red Pitaya C API example Acquiring a signal from a buffer  
 * This application acquires a signal on a specific channel */

#include "lightning-trigger.hpp"

#include <iostream>
#include <signal.h>

using namespace std;

int main(int argc, char **argv)
{
	LightningTrigger t;
	if (t.parseParameters(argc, argv))
		t.run();

	return 0;
}
        
