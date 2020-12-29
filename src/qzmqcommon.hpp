// Copyright 2019 Kasun Hewage
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __QZMQ_COMMON_H__
#define __QZMQ_COMMON_H__

// Handle DSO symbol visibility
// Refer https://gcc.gnu.org/wiki/Visibility for more details

// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
#define QZMQ_HELPER_DLL_IMPORT __declspec(dllimport)
#define QZMQ_HELPER_DLL_EXPORT __declspec(dllexport)
#define QZMQ_HELPER_DLL_LOCAL
#else // defined _WIN32 || defined __CYGWIN__
#if __GNUC__ >= 4
#define QZMQ_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
#define QZMQ_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#define QZMQ_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else // __GNUC__ >= 4
#define QZMQ_HELPER_DLL_IMPORT
#define QZMQ_HELPER_DLL_EXPORT
#define QZMQ_HELPER_DLL_LOCAL
#endif // __GNUC__ >= 4
#endif // defined _WIN32 || defined __CYGWIN__

// Now we use the generic helper definitions above to define QZMQ_API and QZMQ_LOCAL.
// QZMQ_API is used for the public API symbols. 
// It either DLL imports or DLL exports (or does nothing for static build).
// QZMQ_LOCAL is used for non-api symbols.
#ifdef QZMQ_DLL
#ifdef QZMQ_DLL_EXPORTS
#define QZMQ_API QZMQ_HELPER_DLL_EXPORT
#else // QZMQ_DLL_EXPORTS
#define QZMQ_API QZMQ_HELPER_DLL_IMPORT
#endif // QZMQ_DLL_EXPORTS
#define QZMQ_LOCAL QZMQ_HELPER_DLL_LOCAL
#else // QZMQ_DLL
#define QZMQ_API
#define QZMQ_LOCAL
#endif // QZMQ_DLL

#ifdef QZMQ_NAMESPACE
#define QZMQ_BEGIN_NAMESPACE    namespace QZMQ_NAMESPACE {
#define QZMQ_END_NAMESPACE      }
#else
#define QZMQ_BEGIN_NAMESPACE
#define QZMQ_END_NAMESPACE
#endif

#include <cstddef>

#endif // __QZMQ_COMMON_H__