#pragma once
#include "../stdafx.h"
#include <string>
#include <mutex>
#include <vector>

namespace vibration {

	class VibrationController
	{
		class VibrationThreadDeleter {
		public:
			void operator()(std::thread* t) const {
				if (t->joinable()) {
					VibrationController::Reset();
				}
				else
					delete t;
			}
		};

		static std::vector<std::wstring> hidDevPath;
		static std::mutex mtxSync;
		static std::unique_ptr<std::thread, VibrationThreadDeleter> thrVibration;

		VibrationController();
		~VibrationController();

		static void StartVibrationThread(DWORD dwID);
		static void VibrationThreadEntryPoint(DWORD dwID);

	public:
		static void SetHidDevicePath(LPWSTR path);
		static void StartEffect(DWORD dwEffectID, LPCDIEFFECT peff, DWORD dwID);
		static void StopEffect(DWORD dwEffectID);
		static void StopAllEffects();
		static void Reset();
	};

}

