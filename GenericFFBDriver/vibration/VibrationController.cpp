#include "VibrationController.h"
#include <algorithm>
#include <Hidsdi.h>

#define DISABLE_INFINITE_VIBRATION

#define MAX_EFFECTS 5
#define MAXC(a, b) ((a) > (b) ? (a) : (b))

namespace vibration {
	bool quitVibrationThread = false;

	const byte GP_STOP_COMMAND[5] = {
		0x01, 0x01, 0x00, 0x00, 0x00
	};
	void SendHidCommand(HANDLE hHidDevice, const byte* buff, DWORD buffsz) {
		HidD_SetOutputReport(hHidDevice, (PVOID)buff, 5);
	}
	void SendVibrationForce(HANDLE hHidDevice, byte forceSmallMotor, byte forceBigMotor) {
		const byte buffer1[5] = {
			0x01, 0x01, 0x00, forceBigMotor, forceSmallMotor
		};

		SendHidCommand(hHidDevice, (const byte*)&buffer1, 5);
	}
	void SendVibrationStop(HANDLE hHidDevice) {
		SendHidCommand(hHidDevice, (const byte*)&GP_STOP_COMMAND, 5);
	}

	struct VibrationEff {
		DWORD dwEffectId;
		DWORD dwStartFrame;
		DWORD dwStopFrame;

		byte forceX;
		byte forceY;

		BOOL isActive;
		BOOL started;
	};

	VibrationEff VibEffects[MAX_EFFECTS];

	std::vector<std::wstring> VibrationController::hidDevPath;
	std::mutex VibrationController::mtxSync;
	std::unique_ptr<std::thread, VibrationController::VibrationThreadDeleter> VibrationController::thrVibration;

	VibrationController::VibrationController()
	{
	}


	VibrationController::~VibrationController()
	{
	}

	void VibrationController::StartVibrationThread(DWORD dwID)
	{
		mtxSync.lock();

		if (thrVibration == NULL) {
			quitVibrationThread = false;

			for (int k = 0; k < MAX_EFFECTS; k++) {
				VibEffects[k].isActive = FALSE;
				VibEffects[k].dwEffectId = -1;
			}

			thrVibration.reset(new std::thread(VibrationController::VibrationThreadEntryPoint, dwID));
		}

		mtxSync.unlock();
	}

	void VibrationController::VibrationThreadEntryPoint(DWORD dwID)
	{
		// Initialization
		HANDLE hHidDevice = CreateFile(
			hidDevPath[0].c_str(), // Will only use adapter's port 1 for now
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);

		byte lastForceX = 0;
		byte lastForceY = 0;

		while (true) {
			mtxSync.lock();

			if (quitVibrationThread) {
				mtxSync.unlock();
				break;
			}

			DWORD frame = GetTickCount();
			byte forceX = 0;
			byte forceY = 0;

			for (int k = 0; k < MAX_EFFECTS; k++) {
				if (!VibEffects[k].isActive)
					continue;

				if (VibEffects[k].started) {

					if (VibEffects[k].dwStopFrame != INFINITE) {

						if (VibEffects[k].dwStopFrame <= frame) {
							VibEffects[k].isActive = FALSE;
						}
						else {
							forceX = MAXC(forceX, VibEffects[k].forceX);
							forceY = MAXC(forceY, VibEffects[k].forceY);
						}
					}
					else {
						forceX = MAXC(forceX, VibEffects[k].forceX);
						forceY = MAXC(forceY, VibEffects[k].forceY);
					}
				}
				else {
					if (VibEffects[k].dwStartFrame <= frame) {
						VibEffects[k].started = TRUE;
						
						if (VibEffects[k].dwStopFrame != INFINITE) {
							DWORD frmStart = VibEffects[k].dwStartFrame;
							DWORD frmStop = VibEffects[k].dwStopFrame;

							DWORD dt = frmStart <= frmStop ? frmStop - frmStart : frmStart + 100;

							VibEffects[k].dwStopFrame = frame + dt;
						}
#ifdef DISABLE_INFINITE_VIBRATION
						else {
							VibEffects[k].dwStopFrame = frame + 1000;
						}
#endif


						forceX = MAXC(forceX, VibEffects[k].forceX);
						forceY = MAXC(forceY, VibEffects[k].forceY);
					}
				}
			}

			if (forceX != lastForceX || forceY != lastForceY) {
				// Send the command
				if (forceX == 0 && forceY == 0)
					SendVibrationStop(hHidDevice);
				else
					SendVibrationForce(hHidDevice, forceX, forceY);

				lastForceX = forceX;
				lastForceY = forceY;
			}

			mtxSync.unlock();

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		if (hHidDevice != NULL) {
			SendVibrationStop(hHidDevice);
			CloseHandle(hHidDevice);
		}
	}

	void VibrationController::SetHidDevicePath(LPWSTR path)
	{
		hidDevPath.push_back(path);
		Reset();
	}

	void VibrationController::StartEffect(DWORD dwEffectID, LPCDIEFFECT peff, DWORD dwID)
	{
		mtxSync.lock();

		int idx = -1;
		// Reusing the same idx if effect was already created
		for (int k = 0; k < MAX_EFFECTS; k++) {
			if (VibEffects[k].dwEffectId == dwEffectID) {
				idx = k;
				break;
			}
		}

		// Find a non-active idx
		if (idx < 0) {
			for (int k = 0; k < MAX_EFFECTS; k++) {
				if (!VibEffects[k].isActive || k == MAX_EFFECTS - 1) {
					idx = k;
					break;
				}
			}
		}

		// Calculating intensity
		byte forceX = 0xfe;
		byte forceY = 0xfe;

		byte magnitude = 0xfe;
		if (peff->cbTypeSpecificParams == 4) {
			LPDICONSTANTFORCE effParams = (LPDICONSTANTFORCE)peff->lpvTypeSpecificParams;
			double mag = (((double)effParams->lMagnitude) + 10000.0) / 20000.0;

			magnitude = (byte)(round(mag * 254.0));
		}

		if (peff->cAxes == 1) {
			// If direction is negative, then it is a forceX
			// Otherwise it is a forceY
			LONG direction = peff->rglDirection[0];
			static byte lastForceX = 0;
			static byte lastForceY = 0;

			forceX = lastForceX;
			forceY = lastForceY;

			if (direction == -1) {
				//forceX = lastForceX = (byte)(round((((double)peff->dwGain) / 10000.0) * 254.0));
				forceX = lastForceX = magnitude;
			}
			else if (direction == 1) {
				//forceY = lastForceY = (byte)(round((((double)peff->dwGain) / 10000.0) * 254.0));
				forceY = lastForceY = magnitude;
			}

		}
		else {
			if (peff->cAxes >= 1) {
				LONG fx = peff->rglDirection[0];
				//if (fx <= 1) fx = peff->dwGain;

				if (fx > 0)
					forceX = forceY = magnitude;
				else
					forceX = forceY = 0;
			}

			if (peff->cAxes >= 2) {
				LONG fy = peff->rglDirection[1];
				//if (fy <= 1) fy = peff->dwGain;

				if (fy > 0)
					forceY = magnitude;
				else
					forceY = 0;
			}
		}


		DWORD frame = GetTickCount();

		VibEffects[idx].forceX = forceX;
		VibEffects[idx].forceY = forceY;

		VibEffects[idx].dwEffectId = dwEffectID;
		VibEffects[idx].dwStartFrame = frame + (peff->dwStartDelay / 1000);
		VibEffects[idx].dwStopFrame = 
			peff->dwDuration == INFINITE ? INFINITE : 
			VibEffects[idx].dwStartFrame + (peff->dwDuration / 1000);
		VibEffects[idx].isActive = TRUE;
		VibEffects[idx].started = FALSE;

		mtxSync.unlock();
		StartVibrationThread(dwID);
	}

	void VibrationController::StopEffect(DWORD dwEffectID)
	{
		mtxSync.lock();
		for (int k = 0; k < MAX_EFFECTS; k++) {
			if (VibEffects[k].dwEffectId != dwEffectID)
				continue;

			VibEffects[k].dwStopFrame = 0;
		}
		
		mtxSync.unlock();
	}

	void VibrationController::StopAllEffects()
	{
		mtxSync.lock();
		for (int k = 0; k < MAX_EFFECTS; k++) {
			VibEffects[k].dwStopFrame = 0;
		}
		mtxSync.unlock();

		Reset();
	}

	void VibrationController::Reset()
	{
		if (thrVibration == NULL)
			return;

		mtxSync.lock();
		quitVibrationThread = true;
		mtxSync.unlock();

		thrVibration->join();
		thrVibration.reset(NULL);
	}

}
