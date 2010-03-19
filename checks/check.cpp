/*
* (C) 2009 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

/*
 * Test Driver for Botan
 */

#include <vector>
#include <string>

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>

#include <botan/botan.h>
#include <botan/libstate.h>

using namespace Botan;

#include "getopt.h"
#include "bench.h"
#include "validate.h"
#include "common.h"

const std::string VALIDATION_FILE = "checks/validate.dat";
const std::string BIGINT_VALIDATION_FILE = "checks/mp_valid.dat";
const std::string PK_VALIDATION_FILE = "checks/pk_valid.dat";
const std::string EXPECTED_FAIL_FILE = "checks/fail.dat";

int run_test_suite(RandomNumberGenerator& rng);

namespace {

template<typename T>
bool test(const char* type, int digits, bool is_signed)
   {
   if(std::numeric_limits<T>::is_specialized == false)
      {
      std::cout << "WARNING: Could not check parameters of " << type
                << " in std::numeric_limits" << std::endl;

      // assume it's OK (full tests will catch it later)
      return true;
      }

   // continue checking after failures
   bool passed = true;

   if(std::numeric_limits<T>::is_integer == false)
      {
      std::cout << "WARN: std::numeric_limits<> says " << type
                << " is not an integer" << std::endl;
      passed = false;
      }

   if(std::numeric_limits<T>::is_signed != is_signed)
      {
      std::cout << "ERROR: numeric_limits<" << type << ">::is_signed == "
                << std::boolalpha << std::numeric_limits<T>::is_signed
                << std::endl;
      passed = false;
      }

   if(std::numeric_limits<T>::digits != digits && digits != 0)
      {
      std::cout << "ERROR: numeric_limits<" << type << ">::digits == "
                << std::numeric_limits<T>::digits
                << " expected " << digits << std::endl;
      passed = false;
      }

   return passed;
   }

void test_types()
   {
   bool passed = true;

   passed = passed && test<Botan::byte  >("byte",    8, false);
   passed = passed && test<Botan::u16bit>("u16bit", 16, false);
   passed = passed && test<Botan::u32bit>("u32bit", 32, false);
   passed = passed && test<Botan::u64bit>("u64bit", 64, false);
   passed = passed && test<Botan::s32bit>("s32bit", 31,  true);

   if(!passed)
      std::cout << "Typedefs in include/types.h may be incorrect!\n";
   }

}

int main(int argc, char* argv[])
   {
   try
      {
      OptionParser opts("help|test|validate|"
                        "benchmark|bench-type=|bench-algo=|seconds=");
      opts.parse(argv);

      test_types(); // do this always

      Botan::LibraryInitializer init("thread_safe=no");

      if(opts.is_set("help") || argc <= 1)
         {
         std::cerr << "Test driver for "
                   << Botan::version_string() << "\n"
                   << "Options:\n"
                   << "  --test || --validate: Run tests (do this at least once)\n"
                   << "  --benchmark: Benchmark everything\n"
                   << "  --seconds=n: Benchmark for n seconds\n"
                   << "  --init=<str>: Pass <str> to the library\n"
                   << "  --help: Print this message\n";
         return 1;
         }

      Botan::AutoSeeded_RNG rng;

      if(opts.is_set("validate") || opts.is_set("test"))
         {
         return run_test_suite(rng);
         }
      if(opts.is_set("bench-algo") ||
         opts.is_set("benchmark") ||
         opts.is_set("bench-type"))
         {
         double seconds = 5;

         if(opts.is_set("seconds"))
            {
            seconds = std::atof(opts.value("seconds").c_str());
            if(seconds < 0.1 || seconds > (5 * 60))
               {
               std::cout << "Invalid argument to --seconds\n";
               return 2;
               }
            }

         if(opts.is_set("benchmark"))
            {
            benchmark(rng, seconds);
            }
         else if(opts.is_set("bench-algo"))
            {
            std::vector<std::string> algs =
               Botan::split_on(opts.value("bench-algo"), ',');

            for(u32bit j = 0; j != algs.size(); j++)
               {
               const std::string alg = algs[j];
               if(!bench_algo(alg, rng, seconds)) // maybe it's a PK algorithm
                  bench_pk(rng, alg, seconds);
               }
            }
         }
      }
   catch(std::exception& e)
      {
      std::cerr << "Exception: " << e.what() << std::endl;
      return 1;
      }
   catch(...)
      {
      std::cerr << "Unknown (...) exception caught" << std::endl;
      return 1;
      }

   return 0;
   }

int run_test_suite(RandomNumberGenerator& rng)
   {
   std::cout << "Beginning tests..." << std::endl;

   u32bit errors = 0;
   try
      {
      errors += do_validation_tests(VALIDATION_FILE, rng);
      errors += do_validation_tests(EXPECTED_FAIL_FILE, rng, false);
      errors += do_bigint_tests(BIGINT_VALIDATION_FILE, rng);
      errors += do_pk_validation_tests(PK_VALIDATION_FILE, rng);
      //errors += do_cvc_tests(rng);
      }
   catch(std::exception& e)
      {
      std::cout << "Exception: " << e.what() << std::endl;
      return 1;
      }
   catch(...)
      {
      std::cout << "Unknown exception caught." << std::endl;
      return 1;
      }

   if(errors)
      {
      std::cout << errors << " test"  << ((errors == 1) ? "" : "s")
                << " failed." << std::endl;
      return 1;
      }

   std::cout << "All tests passed!" << std::endl;
   return 0;
   }
