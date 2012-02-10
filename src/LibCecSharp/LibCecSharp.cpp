/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "CecSharpTypes.h"
#using <System.dll>

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace CEC;
using namespace msclr::interop;

namespace CecSharp
{
	public ref class LibCecSharp : public CecCallbackMethods
	{
	public:
	  LibCecSharp(LibCECConfiguration ^config)
		{
			m_configuration = config;
			CecCallbackMethods::EnableCallbacks(m_configuration->Callbacks);
			if (!InitialiseLibCec())
				throw gcnew Exception("Could not initialise LibCecSharp");
		}

		LibCecSharp(String ^ strDeviceName, CecDeviceTypeList ^ deviceTypes)
		{
			m_configuration = gcnew LibCECConfiguration();
			m_configuration->SetCallbacks(this);
			m_configuration->DeviceName  = strDeviceName;
			m_configuration->DeviceTypes = deviceTypes;
			if (!InitialiseLibCec())
				throw gcnew Exception("Could not initialise LibCecSharp");
		}
	   
		~LibCecSharp(void)
		{
			Close();
			m_libCec = NULL;
		}

	private:
		!LibCecSharp(void)
		{
			Close();
			m_libCec = NULL;
		}

		bool InitialiseLibCec(void)
		{
			marshal_context ^ context = gcnew marshal_context();
			libcec_configuration config;
			GetConfiguration(context, config);

			m_libCec = (ICECAdapter *) CECInitialise(&config);

			delete context;
			return m_libCec != NULL;
		}

		void GetConfiguration(marshal_context ^context, libcec_configuration &config)
		{
			config.Clear();

			_snprintf_s(config.strDeviceName, 13, context->marshal_as<const char*>(m_configuration->DeviceName));
			for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
				config.deviceTypes.types[iPtr] = (cec_device_type)m_configuration->DeviceTypes->Types[iPtr];

			config.iPhysicalAddress = m_configuration->PhysicalAddress;
			config.baseDevice = (cec_logical_address)m_configuration->BaseDevice;
			config.iHDMIPort = m_configuration->HDMIPort;
			config.clientVersion = (cec_client_version)m_configuration->ClientVersion;
			config.bGetSettingsFromROM = m_configuration->GetSettingsFromROM;
			config.bPowerOnStartup = m_configuration->PowerOnStartup;
			config.bPowerOffShutdown = m_configuration->PowerOffShutdown;
			config.bPowerOffScreensaver = m_configuration->PowerOffScreensaver;
			config.bPowerOffOnStandby = m_configuration->PowerOffOnStandby;
			config.callbacks = &g_cecCallbacks;
		}

	public:
		array<CecAdapter ^> ^ FindAdapters(String ^ path)
		{
			cec_adapter *devices = new cec_adapter[10];

			marshal_context ^ context = gcnew marshal_context();
			const char* strPathC = path->Length > 0 ? context->marshal_as<const char*>(path) : NULL;

			uint8_t iDevicesFound = m_libCec->FindAdapters(devices, 10, NULL);

			array<CecAdapter ^> ^ adapters = gcnew array<CecAdapter ^>(iDevicesFound);
			for (unsigned int iPtr = 0; iPtr < iDevicesFound; iPtr++)
				adapters[iPtr] = gcnew CecAdapter(gcnew String(devices[iPtr].path), gcnew String(devices[iPtr].comm));

			delete devices;
			delete context;
			return adapters;
		}

		bool Open(String ^ strPort, int iTimeoutMs)
		{
			marshal_context ^ context = gcnew marshal_context();
			const char* strPortC = context->marshal_as<const char*>(strPort);
			bool bReturn = m_libCec->Open(strPortC, iTimeoutMs);
			delete context;
			return bReturn;
		}

		void Close(void)
		{
			m_libCec->Close();
		}

		virtual bool EnableCallbacks(CecCallbackMethods ^ callbacks) override
		{
			if (m_libCec && CecCallbackMethods::EnableCallbacks(callbacks))
				return m_libCec->EnableCallbacks(NULL, &g_cecCallbacks);

			return false;
		}

		bool PingAdapter(void)
		{
			return m_libCec->PingAdapter();
		}

		bool StartBootloader(void)
		{
			return m_libCec->StartBootloader();
		}

		int GetMinLibVersion(void)
		{
			return m_libCec->GetMinLibVersion();
		}

		int GetLibVersionMajor(void)
		{
			return m_libCec->GetLibVersionMajor();
		}

		int GetLibVersionMinor(void)
		{
			return m_libCec->GetLibVersionMinor();
		}

		CecLogMessage ^ GetNextLogMessage(void)
		{
			cec_log_message msg;
			if (m_libCec->GetNextLogMessage(&msg))
			{
				return gcnew CecLogMessage(gcnew String(msg.message), (CecLogLevel)msg.level, msg.time);
			}

			return gcnew CecLogMessage();
		}

		CecKeypress ^ GetNextKeypress(void)
		{
			cec_keypress key;
			if (m_libCec->GetNextKeypress(&key))
			{
				return gcnew CecKeypress(key.keycode, key.duration);
			}

			return gcnew CecKeypress();
		}

		CecCommand ^ GetNextCommand(void)
		{
			cec_command command;
			if (m_libCec->GetNextCommand(&command))
			{
				CecCommand ^ retVal = gcnew CecCommand((CecLogicalAddress)command.initiator, (CecLogicalAddress)command.destination, command.ack == 1 ? true : false, command.eom == 1 ? true : false, (CecOpcode)command.opcode, command.transmit_timeout);
				for (uint8_t iPtr = 0; iPtr < command.parameters.size; iPtr++)
					retVal->Parameters->PushBack(command.parameters[iPtr]);
				return retVal;
			}

			return gcnew CecCommand();
		}

		bool Transmit(CecCommand ^ command)
		{
			cec_command ccommand;
			cec_command::Format(ccommand, (cec_logical_address)command->Initiator, (cec_logical_address)command->Destination, (cec_opcode)command->Opcode);
			ccommand.transmit_timeout = command->TransmitTimeout;
			ccommand.eom              = command->Eom;
			ccommand.ack              = command->Ack;
			for (unsigned int iPtr = 0; iPtr < command->Parameters->Size; iPtr++)
				ccommand.parameters.PushBack(command->Parameters->Data[iPtr]);

			return m_libCec->Transmit(ccommand);
		}

		bool SetLogicalAddress(CecLogicalAddress logicalAddress)
		{
			return m_libCec->SetLogicalAddress((cec_logical_address) logicalAddress);
		}

		bool SetPhysicalAddress(int16_t physicalAddress)
		{
			return m_libCec->SetPhysicalAddress(physicalAddress);
		}

		bool PowerOnDevices(CecLogicalAddress logicalAddress)
		{
			return m_libCec->PowerOnDevices((cec_logical_address) logicalAddress);
		}

		bool StandbyDevices(CecLogicalAddress logicalAddress)
		{
			return m_libCec->StandbyDevices((cec_logical_address) logicalAddress);
		}

		bool PollDevice(CecLogicalAddress logicalAddress)
		{
			return m_libCec->PollDevice((cec_logical_address) logicalAddress);
		}

		bool SetActiveSource(CecDeviceType type)
		{
			return m_libCec->SetActiveSource((cec_device_type) type);
		}

		bool SetDeckControlMode(CecDeckControlMode mode, bool sendUpdate)
		{
			return m_libCec->SetDeckControlMode((cec_deck_control_mode) mode, sendUpdate);
		}

		bool SetDeckInfo(CecDeckInfo info, bool sendUpdate)
		{
			return m_libCec->SetDeckInfo((cec_deck_info) info, sendUpdate);
		}

		bool SetInactiveView(void)
		{
			return m_libCec->SetInactiveView();
		}

		bool SetMenuState(CecMenuState state, bool sendUpdate)
		{
			return m_libCec->SetMenuState((cec_menu_state) state, sendUpdate);
		}

		bool SetOSDString(CecLogicalAddress logicalAddress, CecDisplayControl duration, String ^ message)
		{
			marshal_context ^ context = gcnew marshal_context();
			const char* strMessageC = context->marshal_as<const char*>(message);

			bool bReturn = m_libCec->SetOSDString((cec_logical_address) logicalAddress, (cec_display_control) duration, strMessageC);

			delete context;
			return bReturn;
		}

		bool SwitchMonitoring(bool enable)
		{
			return m_libCec->SwitchMonitoring(enable);
		}

		CecVersion GetDeviceCecVersion(CecLogicalAddress logicalAddress)
		{
			return (CecVersion) m_libCec->GetDeviceCecVersion((cec_logical_address) logicalAddress);
		}

		String ^ GetDeviceMenuLanguage(CecLogicalAddress logicalAddress)
		{
			cec_menu_language lang;
			if (m_libCec->GetDeviceMenuLanguage((cec_logical_address) logicalAddress, &lang))
			{
				return gcnew String(lang.language);
			}

			return gcnew String("");
		}

		CecVendorId GetDeviceVendorId(CecLogicalAddress logicalAddress)
		{
			return (CecVendorId)m_libCec->GetDeviceVendorId((cec_logical_address) logicalAddress);
		}

		CecPowerStatus GetDevicePowerStatus(CecLogicalAddress logicalAddress)
		{
			return (CecPowerStatus) m_libCec->GetDevicePowerStatus((cec_logical_address) logicalAddress);
		}

		CecLogicalAddresses ^ GetActiveDevices(void)
		{
			CecLogicalAddresses ^ retVal = gcnew CecLogicalAddresses();
			unsigned int iDevices = 0;

			cec_logical_addresses activeDevices = m_libCec->GetActiveDevices();

			for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
				if (activeDevices[iPtr])
					retVal->Addresses[iDevices++] = (CecLogicalAddress)iPtr;

			return retVal;
		}

		bool IsActiveDevice(CecLogicalAddress logicalAddress)
		{
			return m_libCec->IsActiveDevice((cec_logical_address)logicalAddress);
		}

		bool IsActiveDeviceType(CecDeviceType type)
		{
			return m_libCec->IsActiveDeviceType((cec_device_type)type);
		}

		bool SetHDMIPort(CecLogicalAddress address, uint8_t port)
		{
			return m_libCec->SetHDMIPort((cec_logical_address)address, port);
		}

		uint8_t VolumeUp(bool wait)
		{
			return m_libCec->VolumeUp(wait);
		}

		uint8_t VolumeDown(bool wait)
		{
			return m_libCec->VolumeDown(wait);
		}

		uint8_t MuteAudio(bool wait)
		{
			return m_libCec->MuteAudio(wait);
		}

		bool SendKeypress(CecLogicalAddress destination, CecUserControlCode key, bool wait)
		{
			return m_libCec->SendKeypress((cec_logical_address)destination, (cec_user_control_code)key, wait);
		}

		bool SendKeyRelease(CecLogicalAddress destination, bool wait)
		{
			return m_libCec->SendKeyRelease((cec_logical_address)destination, wait);
		}

		String ^ GetDeviceOSDName(CecLogicalAddress logicalAddress)
		{
			cec_osd_name osd = m_libCec->GetDeviceOSDName((cec_logical_address) logicalAddress);
			return gcnew String(osd.name);
		}

		CecLogicalAddress GetActiveSource()
		{
			return (CecLogicalAddress)m_libCec->GetActiveSource();
		}

		bool IsActiveSource(CecLogicalAddress logicalAddress)
		{
			return m_libCec->IsActiveSource((cec_logical_address)logicalAddress);
		}

		uint16_t GetDevicePhysicalAddress(CecLogicalAddress iAddress)
		{
			return m_libCec->GetDevicePhysicalAddress((cec_logical_address)iAddress);
		}

		bool GetCurrentConfiguration(LibCECConfiguration ^configuration)
		{
			libcec_configuration config;
			if (m_libCec->GetCurrentConfiguration(&config))
			{
				configuration->BaseDevice = (CecLogicalAddress)config.baseDevice;
				configuration->DeviceName = gcnew String(config.strDeviceName);
				configuration->HDMIPort = config.iHDMIPort;
				configuration->PhysicalAddress = config.iPhysicalAddress;
				configuration->PowerOffOnStandby = config.bPowerOffOnStandby == 1;
				configuration->PowerOffScreensaver = config.bPowerOffScreensaver == 1;
				configuration->PowerOffShutdown = config.bPowerOffShutdown == 1;
				configuration->PowerOnStartup = config.bPowerOnStartup == 1;
				configuration->UseTVMenuLanguage = config.bUseTVMenuLanguage == 1;
				for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
					m_configuration->DeviceTypes->Types[iPtr] = (CecDeviceType)config.deviceTypes.types[iPtr];
				return true;
			}
			return false;
		}

    bool CanPersistConfiguration(void)
		{
			return m_libCec->CanPersistConfiguration();
		}

    bool PersistConfiguration(LibCECConfiguration ^configuration)
		{
			marshal_context ^ context = gcnew marshal_context();
			libcec_configuration config;
			GetConfiguration(context, config);

			bool bReturn = m_libCec->PersistConfiguration(&config);

			delete context;
			return bReturn;
		}

		String ^ ToString(CecLogicalAddress iAddress)
		{
			const char *retVal = m_libCec->ToString((cec_logical_address)iAddress);
			return gcnew String(retVal);
		}

		String ^ ToString(CecVendorId iVendorId)
		{
			const char *retVal = m_libCec->ToString((cec_vendor_id)iVendorId);
			return gcnew String(retVal);
		}
	  
		String ^ ToString(CecVersion iVersion)
		{
			const char *retVal = m_libCec->ToString((cec_version)iVersion);
			return gcnew String(retVal);
		}
	  
		String ^ ToString(CecPowerStatus iState)
		{
			const char *retVal = m_libCec->ToString((cec_power_status)iState);
			return gcnew String(retVal);
		}

		String ^ ToString(CecMenuState iState)
		{
			const char *retVal = m_libCec->ToString((cec_menu_state)iState);
			return gcnew String(retVal);
		}

		String ^ ToString(CecDeckControlMode iMode)
		{
			const char *retVal = m_libCec->ToString((cec_deck_control_mode)iMode);
			return gcnew String(retVal);
		}

		String ^ ToString(CecDeckInfo status)
		{
			const char *retVal = m_libCec->ToString((cec_deck_info)status);
			return gcnew String(retVal);
		}

		String ^ ToString(CecOpcode opcode)
		{
			const char *retVal = m_libCec->ToString((cec_opcode)opcode);
			return gcnew String(retVal);
		}

		String ^ ToString(CecSystemAudioStatus mode)
		{
			const char *retVal = m_libCec->ToString((cec_system_audio_status)mode);
			return gcnew String(retVal);
		}

		String ^ ToString(CecAudioStatus status)
		{
			const char *retVal = m_libCec->ToString((cec_audio_status)status);
			return gcnew String(retVal);
		}

	private:
		ICECAdapter *        m_libCec;
		LibCECConfiguration ^m_configuration;
	};
}
