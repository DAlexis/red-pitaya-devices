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
	constexpr static rp_dpin_t ttlPin = RP_DIO0_P;
	static const std::string timeTag;
	static const std::string numberTag;

	static std::string getTime();
	static std::string replaceTags(const std::string& tmpl, int n);
	static rp_acq_decimation_t convertDecimation(unsigned int value);

	double m_treshold = 0.1;
	double m_ttlPulse = 0.1;
	unsigned int m_capuresCount = 0;
	std::string m_filenameTemplate = "recorded-field-%n-%t.bin";
	bool m_fileEnabled = false;
	rp_acq_decimation_t m_decimation;
	int m_decimationValue = 8;
	bool m_onlyPrintHelp = false;

	bool m_shouldStop = false;
	std::vector<float> m_buffer;
};


#endif /* LIGHTNING_TRIGGER_LIGHTNING_TRIGGER_HPP_ */
