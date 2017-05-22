/*
 * lightning-trigger.cpp
 *
 *  Created on: 18 May 2017
 *      Author: Aleksey Bulatov
 */

#include "trigger-recorder.hpp"

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

const std::string TriggerRecorder::timeTag = "%t";
const std::string TriggerRecorder::numberTag = "%n";

bool TriggerRecorder::parseParameters(int argc, char **argv)
{
	namespace po = boost::program_options;
	boost::program_options::variables_map cmdLineOptions;
	po::options_description generalOptions("General options");
	generalOptions.add_options()
		("help,h", "Print help message")
		("captures-cout,n",   po::value<unsigned int>()->default_value(m_capuresCount), "Count of captures (0 for infinite)")
		("config,c", po::value<string>(), "Use configuration file");

	po::options_description captureOptions("Capture options");
	captureOptions.add_options()
		("threshold,t",  po::value<double>()->default_value(m_treshold), "Trigger treshold, [0..1]")
		("decimation,d", po::value<unsigned int>()->default_value(m_decimationValue),  "Field decimation. Allowed: "
				"1, 8, 64, 1024, 8192, 65536");

	po::options_description outputOptions("Output options");
	outputOptions.add_options()
		("pulse-width,w", po::value<double>()->default_value(m_ttlPulse), "Output TTL pulse width, s")
		("field-file,f", po::value<string>()->default_value(m_filenameTemplate), "File to store electric field")
		("silent,S", "Silent mode: no text 'Triggered' text output")
		("save-field,s", "Enable field saving");

	po::options_description allOptions("Allowed options");
	allOptions
		.add(generalOptions)
		.add(captureOptions)
		.add(outputOptions);

	try
	{
		po::store(po::parse_command_line(argc, argv, allOptions), cmdLineOptions);
		po::notify(cmdLineOptions);
		m_treshold = cmdLineOptions["threshold"].as<double>();
		if (m_treshold > 1.0)
			m_treshold = 1.0;
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

	if (cmdLineOptions.count("help") != 0)
	{
		cout << "Simple software for RedPitaya device that may be used as a console oscilloscope" << endl << endl;
		cout << allOptions << endl;
		return false;
	}

	m_silent = (cmdLineOptions.count("silent") != 0);

	if (cmdLineOptions.count("config") != 0)
	{
		return tryReadConfigFile(cmdLineOptions["config"].as<string>() );
	}

	m_fileEnabled = (cmdLineOptions.count("save-field") != 0);

	return true;
}

bool TriggerRecorder::tryReadConfigFile(const std::string& filename)
{
	boost::property_tree::ptree pt;
	try {
		boost::property_tree::ini_parser::read_ini("/etc/rp-trigger-recorder.conf", pt);
		m_treshold = pt.get<double>("capture.threshold");
		if (m_treshold > 1.0)
			m_treshold = 1.0;

		m_decimationValue = pt.get<unsigned int>("capture.decimation");
		m_decimation = convertDecimation(m_decimationValue);

		m_ttlPulse = pt.get<double>("output.pulse-width");
		m_filenameTemplate =  pt.get<string>("output.field-file");

		m_capuresCount = pt.get<unsigned int>("general.captures-count");
		m_fileEnabled = pt.get<bool>("general.save-field");
	}
	catch(boost::property_tree::ini_parser::ini_parser_error &exception)
	{
		cerr << std::string("Parsing error in ") + exception.filename() << endl;
		return false;
	}
	catch(boost::property_tree::ptree_error &exception)
	{
		cerr << std::string("Parsing error in ") + exception.what() << endl;
		return false;
	}
	catch(...)
	{
		cerr << "Unknown parsing error" << endl;
		return false;
	}

	return true;
}

void TriggerRecorder::run()
{
	// Print error, if rp_Init() function failed
	if (rp_Init() != RP_OK)
	{
	   cerr << "RedPitaya API initialization failed!" << endl;
	   return;
	}

	rp_DpinSetDirection(ttlPin, RP_OUT);

	m_blinkingThread.reset(new thread([this]{ blinkingTask(); }));

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

		std::this_thread::sleep_for( std::chrono::milliseconds(size_t(2 * getBufferDuration()*1000)));
		rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
		rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

		while(!m_shouldStop)
		{
			rp_AcqGetTriggerState(&state);
			if(state == RP_TRIG_STATE_TRIGGERED)
			{
				if (!m_silent)
					cout << "Triggered" << endl;
				break;
			}
		}

		std::this_thread::sleep_for (std::chrono::milliseconds(size_t(2 * getBufferDuration()*1000)));

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

	stop();

	rp_Release();
}

void TriggerRecorder::stop()
{
	m_shouldStop = true;
	doBlink();
	if (m_blinkingThread->joinable())
		m_blinkingThread->join();
}

void TriggerRecorder::doBlink()
{
	unique_lock<mutex> lock(m_blinkMutex);
	m_blinkReady = true;
	m_blinkCV.notify_one();
}

void TriggerRecorder::blinkingTask()
{
	for(;;)
	{
		unique_lock<mutex> lock(m_blinkMutex);
		while (!m_blinkReady) m_blinkCV.wait(lock);
		m_blinkReady = false;
		lock.unlock();

		if (m_shouldStop)
			return;

		rp_DpinSetState(ledPin, RP_HIGH);
		rp_DpinSetState(ttlPin, RP_HIGH);
		std::this_thread::sleep_for (std::chrono::milliseconds(size_t(1000*m_ttlPulse)));
		rp_DpinSetState(ledPin, RP_LOW);
		rp_DpinSetState(ttlPin, RP_LOW);
		std::this_thread::sleep_for (std::chrono::milliseconds(size_t(1000*m_ttlPulse)));
	}
}

double TriggerRecorder::getBufferDuration()
{
	return double(m_decimationValue) / 125e6 * bufferSize;
}

std::string TriggerRecorder::getTime()
{
	ostringstream oss;
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
	return oss.str();
}

std::string TriggerRecorder::replaceTags(const std::string& tmpl, int n)
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

rp_acq_decimation_t TriggerRecorder::convertDecimation(unsigned int value)
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
