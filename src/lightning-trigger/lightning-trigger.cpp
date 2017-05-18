/*
 * lightning-trigger.cpp
 *
 *  Created on: 18 May 2017
 *      Author: Aleksey Bulatov
 */

#include "lightning-trigger.hpp"

#include <boost/program_options.hpp>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

const std::string LightningTrigger::timeTag = "%t";
const std::string LightningTrigger::numberTag = "%n";

bool LightningTrigger::parseParameters(int argc, char **argv)
{
	namespace po = boost::program_options;
	boost::program_options::variables_map cmdLineOptions;
	po::options_description generalOptions("General options");
	generalOptions.add_options()
		("help,h", "Print help message")
		("captures-cout,c",   po::value<unsigned int>()->default_value(m_capuresCount), "Count of captures (0 for infinite)");

	po::options_description captureOptions("Capture options");
	captureOptions.add_options()
		("threshold,t",   po::value<double>()->default_value(m_treshold), "Trigger treshold, Volts")
		("decimation,d",  po::value<unsigned int>()->default_value(m_decimationValue),  "Field decimation. Allowed: "
				"1, 8, 64, 1024, 8192, 65536");

	po::options_description outputOptions("Output options");
	outputOptions.add_options()
		("pulse-width,w", po::value<double>()->default_value(m_ttlPulse), "Output TTL pulse width, s")
		("field-file,f", po::value<string>()->default_value(m_filenameTemplate), "File to store electric field")
		("save-field,s", "Enable field saving");

	po::options_description allOptions("Allowed options");
	allOptions
		.add(generalOptions)
		.add(captureOptions)
		.add(outputOptions);

	cout << "Parsing cmdline" << endl;
	try
	{
		po::store(po::parse_command_line(argc, argv, allOptions), cmdLineOptions);
		po::notify(cmdLineOptions);
		m_treshold = cmdLineOptions["threshold"].as<double>();
		m_ttlPulse = cmdLineOptions["pulse-width"].as<double>();
		m_decimationValue = cmdLineOptions["decimation"].as<unsigned int>();
		m_decimation = convertDecimation(m_decimationValue);
		m_filenameTemplate = cmdLineOptions["field-file"].as<string>();
		m_capuresCount = cmdLineOptions["captures-cout"].as<unsigned int>();
	}
	catch (po::error& e)
	{
		cerr << "Command line parsing error: " << e.what() << endl;
		return false;
	}
	catch (exception& e)
	{
		cerr << "Bad arguments: " << e.what() << endl;
		return false;
	}

	m_fileEnabled = (cmdLineOptions.count("save-field") != 0);

	m_onlyPrintHelp = (cmdLineOptions.count("help") != 0);

	if (m_onlyPrintHelp)
	{
		cout << allOptions << endl;
		return false;
	}
	return true;
}

void LightningTrigger::run()
{
	//return 0;

	// Print error, if rp_Init() function failed
	if (rp_Init() != RP_OK)
	{
	   cerr << "RedPitaya API initialization failed!" << endl;
	}

	cout << "RedPitaya API initialization done" << endl;

	m_buffer.resize(bufferSize, 0.0);

	int result = RP_OK;


	for (unsigned int c=0; !m_shouldStop && (m_capuresCount == 0 || c != m_capuresCount); c++)
	{
		rp_AcqReset();
		rp_AcqSetDecimation(m_decimation);
		rp_AcqSetTriggerLevel(RP_CH_1, m_treshold);
		rp_AcqSetTriggerDelay(0);
		rp_AcqStart();

		// After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer
		// Here we have used time delay of one second but you can calculate exact value taking in to account buffer
		//length and smaling rate

		//usleep(double(m_decimationValue) / 125e6 * bufferSize * 1000 * 1.5);
		usleep(double(m_decimationValue) / 125e6 * bufferSize * 1000 * 3);
		rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
		rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

		while(!m_shouldStop)
		{
			rp_AcqGetTriggerState(&state);
			if(state == RP_TRIG_STATE_TRIGGERED)
			{
				cout << "Triggered" << endl;
				break;
			}
		}

		if (m_shouldStop)
			break;

		std::fill(m_buffer.begin(), m_buffer.end(), 0);

		uint32_t readed = bufferSize;
		result = rp_AcqGetOldestDataV(RP_CH_1, &readed, m_buffer.data());
		if (result != RP_OK)
		{
			cerr << "Error: rp_AcqGetOldestDataV result is " << result << endl;
		}

		doBlink();

		if (m_fileEnabled)
		{
			string filename = replaceTags(m_filenameTemplate, c);
			ofstream outpf(filename, ios::binary | ios::out);
			if (!outpf.good())
			{
				cerr << "Cannot open file " << filename << endl;
				continue;
			}

			outpf.write(reinterpret_cast<char*>(m_buffer.data()), readed * sizeof(m_buffer[0]));
		}
	}
	// Releasing resources
	rp_Release();
}

void LightningTrigger::stop()
{
	m_shouldStop = true;
}

void LightningTrigger::doBlink()
{
	rp_DpinSetState(ledPin, RP_HIGH);
	rp_DpinSetState(ttlPin, RP_HIGH);
	usleep(m_ttlPulse*1000);
	rp_DpinSetState(ledPin, RP_LOW);
	rp_DpinSetState(ttlPin, RP_LOW);
}

std::string LightningTrigger::getTime()
{
	ostringstream oss;
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
	return oss.str();
}

std::string LightningTrigger::replaceTags(const std::string& tmpl, int n)
{
	string result = tmpl;
	size_t pos = result.find(timeTag);
	if (pos != string::npos)
		result.replace(pos, timeTag.length(), getTime());

	pos = result.find(numberTag);
	if (pos != string::npos)
		result.replace(pos, numberTag.length(), to_string(n));
	return result;
}

rp_acq_decimation_t LightningTrigger::convertDecimation(unsigned int value)
{
	switch(value)
	{
	case 1:
		return RP_DEC_1;     // Sample rate 125Msps; Buffer time length 131us; Decimation 1
	case 8:
		return RP_DEC_8;     // Sample rate 15.625Msps; Buffer time length 1.048ms; Decimation 8
	case 64:
		return RP_DEC_64;    // Sample rate 1.953Msps; Buffer time length 8.388ms; Decimation 64
	case 1024:
		return  RP_DEC_1024;  // Sample rate 122.070ksps; Buffer time length 134.2ms; Decimation 1024
	case 8192:
		return  RP_DEC_8192;  // Sample rate 15.258ksps; Buffer time length 1.073s; Decimation 8192
	case 65536:
		return  RP_DEC_65536;  // Sample rate 1.907ksps; Buffer time length 8.589s; Decimation 65536
	}
	throw std::range_error(std::string("Invalid decimation rate: " + to_string(value)));
}
