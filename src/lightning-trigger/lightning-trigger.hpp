/*
 * lightning-trigger.hpp
 *
 *  Created on: 18 May 2017
 *      Author: Aleksey Bulatov
 */

#ifndef LIGHTNING_TRIGGER_LIGHTNING_TRIGGER_HPP_
#define LIGHTNING_TRIGGER_LIGHTNING_TRIGGER_HPP_

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <condition_variable>

#include "redpitaya/rp.h"

class LightningTrigger
{
public:
	bool parseParameters(int argc, char **argv);
	void run();
	void stop();
	void doBlink();

private:
	constexpr static size_t bufferSize = 16384;
	constexpr static rp_dpin_t ledPin = RP_LED0;
	constexpr static rp_dpin_t ttlPin = RP_DIO0_N;
	static const std::string timeTag;
	static const std::string numberTag;

	static std::string getTime();
	static std::string replaceTags(const std::string& tmpl, int n);
	static rp_acq_decimation_t convertDecimation(unsigned int value);

	double getBufferDuration();
	void blinkingTask();

	double m_treshold = 0.1;
	double m_ttlPulse = 0.1;
	unsigned int m_capuresCount = 0;
	std::string m_filenameTemplate = "recorded-field-%t-%n.bin";
	bool m_fileEnabled = false;
	rp_acq_decimation_t m_decimation;
	int m_decimationValue = 8;
	bool m_onlyPrintHelp = false;

	bool m_shouldStop = false;
	std::vector<float> m_buffer;

	std::unique_ptr<std::thread> m_blinkingThread;
	std::mutex m_blinkMutex;
	std::condition_variable m_blinkCV;
	bool m_blinkReady = false;
};


#endif /* LIGHTNING_TRIGGER_LIGHTNING_TRIGGER_HPP_ */
