
#if !defined(_REGISTRAR_H)
#define _REGISTRAR_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "stdio.h"

class CRegistrar
{
protected:
	CRegistrar() {};

	BOOL SetInRegistry(HKEY hRootKey, LPCSTR subKey, LPCSTR keyName,LPCSTR keyValue)
	{
		HKEY hKeyResult;
		size_t dataLength;
		DWORD dwDisposition;
		if (RegCreateKeyExA( hRootKey, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
							KEY_WRITE, NULL, &hKeyResult, &dwDisposition) != ERROR_SUCCESS)
		{
			return FALSE;
		}
		dataLength = strlen(keyValue);
		DWORD retVal = RegSetValueExA( hKeyResult, keyName, 0, REG_SZ,(const BYTE *) keyValue, (DWORD)dataLength);
		RegCloseKey(hKeyResult);
		return (retVal == ERROR_SUCCESS) ? TRUE:FALSE;
	}
	BOOL SetInRegistry(HKEY hRootKey, LPCSTR subKey, LPCSTR keyName, const BYTE* keyValue, DWORD keyValueLen)
	{
		HKEY hKeyResult;
		DWORD dwDisposition;
		if (RegCreateKeyExA(hRootKey, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &hKeyResult, &dwDisposition) != ERROR_SUCCESS)
		{
			return FALSE;
		}
		DWORD retVal = RegSetValueExA(hKeyResult, keyName, 0, REG_BINARY, keyValue, keyValueLen);
		RegCloseKey(hKeyResult);
		return (retVal == ERROR_SUCCESS) ? TRUE : FALSE;
	}

	BOOL DelFromRegistry(HKEY hRootKey, LPCSTR subKey)
	{
		long retCode;
		retCode = RegDeleteKeyA(hRootKey, subKey);
		if (retCode != ERROR_SUCCESS)
			return false;
		return true;
	}

	bool StrFromCLSID(REFIID riid,LPSTR strCLSID)
	{
		sprintf_s(strCLSID, MAX_PATH, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
			riid.Data1, riid.Data2, riid.Data3,
			riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
			riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
		
		return true;
	}
public:
	bool RegisterObject(REFIID riid,LPCSTR LibId,LPCSTR ClassId)
	{
		char strCLSID [ MAX_PATH ];
		char buffer [ MAX_PATH ];
		
		if(!strlen(ClassId))
			return false;

		if(!StrFromCLSID(riid,strCLSID))
			return false;
		
		if(!strlen(LibId) && strlen(ClassId))
			sprintf_s(buffer,"%s.%s\\CLSID",ClassId,ClassId);
		else
			sprintf_s(buffer,"%s.%s\\CLSID",LibId,ClassId);

		BOOL result;
		result = SetInRegistry(HKEY_CLASSES_ROOT,buffer,"",strCLSID);
		if(!result)
			return false;
		sprintf_s(buffer,"CLSID\\%s",strCLSID);
		char Class [ MAX_PATH ];
		sprintf_s(Class,"%s Class",ClassId);
		if(!SetInRegistry(HKEY_CLASSES_ROOT,buffer,"",Class))
			return false;
		sprintf_s(Class,"%s.%s",LibId,ClassId);
		strcat_s(buffer,"\\ProgId");

		return SetInRegistry(HKEY_CLASSES_ROOT,buffer,"",Class) ? true:false;
	}

	bool UnRegisterObject(REFIID riid,LPCSTR LibId,LPCSTR ClassId)
	{
		char strCLSID [ MAX_PATH ];
		char buffer [ MAX_PATH ];
		if(!StrFromCLSID(riid,strCLSID))
			return false;
		sprintf_s(buffer,"%s.%s\\CLSID",LibId,ClassId);
		if(!DelFromRegistry(HKEY_CLASSES_ROOT,buffer))
			return false;
		sprintf_s(buffer,"%s.%s",LibId,ClassId);
		if(!DelFromRegistry(HKEY_CLASSES_ROOT,buffer))
			return false;
		sprintf_s(buffer,"CLSID\\%s\\ProgId",strCLSID);
		if(!DelFromRegistry(HKEY_CLASSES_ROOT,buffer))
			return false;
		sprintf_s(buffer,"CLSID\\%s",strCLSID);
		return DelFromRegistry(HKEY_CLASSES_ROOT,buffer) ? true:false;
	}
};

class CDllRegistrar : public CRegistrar
{
public:
	bool RegisterObject(REFIID riid,LPCSTR LibId,LPCSTR ClassId,LPCSTR Path)
	{
		if(! CRegistrar::RegisterObject(riid,LibId,ClassId))
			return false;

		char strCLSID [ MAX_PATH ];
		char buffer [ MAX_PATH ];
		if(!StrFromCLSID(riid,strCLSID))
			return false;
		
		sprintf_s(buffer, "CLSID\\%s\\InProcServer32", strCLSID);
		if(! SetInRegistry(HKEY_CLASSES_ROOT,buffer,"",Path))
			return false;

		if(! SetInRegistry(HKEY_CLASSES_ROOT, buffer, "ThreadingModel", "Both"))
			return false;

	// Root ----------------
		const char* root = "SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_0D9D&PID_3012";
		const byte oemData[] = { 
			0x03, 0x00, 0x08, 0x10, 0x0c, 0x00, 0x00, 0x00
		};
		byte dData[] = {
			0x00, 0x00, 0x00, 0x00
		};

		if (!SetInRegistry(HKEY_LOCAL_MACHINE, root, "OEMName", "DualShock Adapter"))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, root, "OEMData", oemData, 8))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, root, "DebugLevel", dData, 4))
			return false;
		dData[0] = 0x19;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, root, "Delay", dData, 4))
			return false;
		dData[0] = 0xe8; dData[1] = 0x03;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, root, "Amplify", dData, 4))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, root, "ConfigCLSID", "{B4FE8B13-40D0-438A-B4C2-DE4522951071}"))
			return false;
	// Root ----------------

	// AXES ----------------	
		const char* axePath = "SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_0D9D&PID_3012\\Axes";
		byte axeAttrData[] = {
			0x01, 0x81, 0x00, 0x00, 0x01, 0x00, 0x30, 0x00
		};
		const byte axeFFAttrData[] = {
			0x0a, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
		};

		sprintf_s(buffer, "%s\\%d", axePath, 0);
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "X axis"))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", axeAttrData, 8))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "FFAttributes", axeFFAttrData, 8))
			return false;

		sprintf_s(buffer, "%s\\%d", axePath, 1);
		axeAttrData[6] = 0x31;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Y axis"))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", axeAttrData, 8))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "FFAttributes", axeFFAttrData, 8))
			return false;

		sprintf_s(buffer, "%s\\%d", axePath, 2);
		axeAttrData[6] = 0x32;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Z axis"))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", axeAttrData, 8))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "FFAttributes", axeFFAttrData, 8))
			return false;

		sprintf_s(buffer, "%s\\%d", axePath, 5);
		axeAttrData[6] = 0x35;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Rz axis"))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", axeAttrData, 8))
			return false;
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "FFAttributes", axeFFAttrData, 8))
			return false;
	// AXES ----------------

	// Buttons -------------
		const char* btnPath = "SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_0D9D&PID_3012\\Buttons";
		byte btnData[] = {
			0x02, 0x80, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00
		};

		for (int i = 0; i < 12; i++)
		{
			sprintf_s(buffer, "%s\\%d", btnPath, i);
			btnData[6] = i+1;
			if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", btnData, 8))
				return false;
		}
	// Buttons -------------
	
	// ForceFeedback -------
		const char* oemPath = "SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_0D9D&PID_3012\\OEMForceFeedback";

		if (!SetInRegistry(HKEY_LOCAL_MACHINE, oemPath, "CLSID", "{B4FE8B13-40D0-438A-B4C2-DE4522951071}"))
			return false;

		const byte attrVal[] = {
			0x00, 0x00, 0x00, 0x00,
			0xe8, 0x03, 0x00, 0x00,
			0xe8, 0x03, 0x00, 0x00
		};
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, oemPath, "Attributes", attrVal, 12))
			return false;
	// ForceFeedback -------

	// Effects -------------
		sprintf_s(buffer, "%s\\Effects", oemPath);
		if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", ""))
			return false;

		for (int i = 0; i < 12; i++)
		{
			sprintf_s(buffer, "%s\\Effects\\{13541C2%01X-8E33-11D0-9AD0-00A0C9A06E35}", oemPath, i);

			switch (i)
			{
				case 0:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Constant"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x00,0x00,0x00,0x00,0x01,0x86,0x00,0x00,0xed,0x03,0x00,0x00,0xed,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 1:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Ramp Force"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x01,0x00,0x00,0x00,0x02,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 2:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Square Wave"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x02,0x00,0x00,0x00,0x03,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 3:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Sine Wave"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x03,0x00,0x00,0x00,0x03,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 4:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Triangle Wave"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x04,0x00,0x00,0x00,0x03,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 5:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Sawtooth Up Wave"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x05,0x00,0x00,0x00,0x03,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 6:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Sawtooth Down Wave"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x06,0x00,0x00,0x00,0x03,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 7:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Spring"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x07,0x00,0x00,0x00,0x04,0xd8,0x00,0x00,0x6d,0x03,0x00,0x00,0x6d,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 8:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Damper"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x08,0x00,0x00,0x00,0x04,0xd8,0x00,0x00,0x6d,0x03,0x00,0x00,0x6d,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 9:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Inertia"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x09,0x00,0x00,0x00,0x04,0xd8,0x00,0x00,0x6d,0x03,0x00,0x00,0x6d,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 10:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "Friction"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x0a,0x00,0x00,0x00,0x04,0xd8,0x00,0x00,0x6d,0x03,0x00,0x00,0x6d,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
				case 11:
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "", "CustomForce"))
						return false;
					if (!SetInRegistry(HKEY_LOCAL_MACHINE, buffer, "Attributes", new byte[]{
						0x00,0x01,0x00,0x00,0x05,0x86,0x00,0x00,0xef,0x03,0x00,0x00,0xef,0x03,0x00,0x00,0x30,0x00,0x00,0x00
						}, 20))
						return false;
					break;
			}
		}

	// Effects -------------

		return true;
	}

	bool UnRegisterObject(REFIID riid,LPCSTR LibId,LPCSTR ClassId)
	{
		char strCLSID [ MAX_PATH ];
		char buffer [ MAX_PATH ];
		if(!StrFromCLSID(riid,strCLSID))
			return false;
		sprintf_s(buffer,"CLSID\\%s\\InProcServer32",strCLSID);
		if(!DelFromRegistry(HKEY_CLASSES_ROOT,buffer))
			return false;
		return CRegistrar::UnRegisterObject(riid,LibId,ClassId);
	}

};

#endif