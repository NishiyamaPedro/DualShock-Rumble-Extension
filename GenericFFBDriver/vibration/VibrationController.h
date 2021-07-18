#pragma once
#include "../stdafx.h"
#include <string>
#include <mutex>
#include <vector>
#include <map>

namespace vibration {

	class VibrationController
	{
		class VibrationThreadDeleter {
		public:
			void operator()(std::thread* t) const {
				if (t->joinable()) {
					VibrationController::Reset(0, t);
				}
				else
					delete t;
			}
		};

		static std::vector<std::wstring> hidDevPath;
		static std::mutex mtxSync;
		static std::unique_ptr<std::thread, VibrationThreadDeleter> thrVibration[2];
		
		VibrationController();
		~VibrationController();

		static void StartVibrationThread(DWORD dwID);
		static void VibrationThreadEntryPoint(DWORD dwID);

	public:
		static void SetHidDevicePath(LPWSTR path, DWORD dwID);
		static void StartEffect(DWORD dwEffectID, LPCDIEFFECT peff, DWORD dwID);
		static void StopEffect(DWORD dwEffectID, DWORD dwID);
		static void StopAllEffects(DWORD dwID);
		static void Reset(DWORD dwID, std::thread* t = NULL);
	};

}

