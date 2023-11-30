#pragma once

#ifdef BUILDING_ML_DLL
	#ifdef __GNUC__
		#define ML_DLL __attribute__((visibility("default")))
	#elif _WIN32
		#define ML_DLL __declspec(dllexport)
	#else
		#error not supported on compiler
	#endif // __GNUC__
#else
	#ifdef __GNUC__
		#define ML_DLL
	#elif _WIN32
		#define ML_DLL __declspec(dllimport)
	#else
		#error not supported on compiler
	#endif // __GNUC__
#endif // BUILDING_ML_DLL
