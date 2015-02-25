#include "framework/buffers/NormalisedSizeBuffer.h"
#include "common.h"

void dprint(int *d) {
	cout << "---" << endl;
	for(int i = 0; i < 8; i++)
		cout << d[i] << " ";

	cout << endl;
}
int main( void ) {
	RackoonIO::NormalisedSizeBuffer<int> buffer(8, (16<<2)-1);
	int testA[22], testB[27];
	int j = 1;
	for(int i = 0; i < 22; i++)
		testA[i] = j++;

	for(int i = 0; i < 27; i++)
		testB[i] = j++;

	int *dispatch;

	RackoonIO::NormalisedSizeBufferState cState = RackoonIO::NormalisedSizeBufferState::DISPATCH;

	buffer.supply(testA, 22);
	buffer.reset();

	while(j < 49<<3) {
		// First check
		if(buffer.getState() == RackoonIO::NormalisedSizeBufferState::DISPATCH) {
			dispatch = buffer.flush();
			dprint(dispatch);
			continue;
		}

		// if we're here then the buffer is
		// partial
		if((cState = buffer.supply(testB, 27)) == RackoonIO::NormalisedSizeBufferState::DISPATCH) {
			dispatch = buffer.flush();
			dprint(dispatch);
		} else if(cState == RackoonIO::NormalisedSizeBufferState::OVERFLOW)
			cout << "Overflow Occurred" << endl;

		for(int i = 0; i < 27; i++)
			testB[i] = j++;
		
	}

	cout << "Done!" << endl;

}