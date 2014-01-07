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

#include <botan/init.h>
#include <botan/version.h>
#include <botan/auto_rng.h>
#include <botan/cpuid.h>
#include <botan/http_util.h>

using namespace Botan;

#include "tests/tests.h"
#include "apps/apps.h"

namespace {

int help(int , char* argv[])
   {
   std::cout << "Usage: " << argv[0] << " subcommand\n";
   std::cout << "Common commands: test help version\n";
   std::cout << "Other commands: speed cpuid bcrypt x509 factor tls_client asn1 base64 hash self_sig\n";
   return 1;
   }

}

int main(int argc, char* argv[])
   {
   if(BOTAN_VERSION_MAJOR != version_major() ||
      BOTAN_VERSION_MINOR != version_minor() ||
      BOTAN_VERSION_PATCH != version_patch())
      {
      std::cout << "Warning: linked version ("
                << version_major() << '.'
                << version_minor() << '.'
                << version_patch()
                << ") does not match version built against ("
                << BOTAN_VERSION_MAJOR << '.'
                << BOTAN_VERSION_MINOR << '.'
                << BOTAN_VERSION_PATCH << ")\n";
      }

   try
      {
      Botan::LibraryInitializer init;

      if(argc < 2)
         return help(argc, argv);

      const std::string cmd = argv[1];

      if(cmd == "help")
         return help(argc, argv);

      if(cmd == "version")
         {
         std::cout << Botan::version_string() << "\n";
         return 0;
         }

      if(cmd == "cpuid")
         {
         CPUID::print(std::cout);
         return 0;
         }

      if(cmd == "test")
         return test_main(argc - 1, argv + 1);

      if(cmd == "speed")
         return speed_main(argc - 1, argv + 1);

      if(cmd == "http_get")
         {
         auto resp = HTTP::GET_sync(argv[2]);
         std::cout << resp << "\n";
         }

      int e = apps_main(cmd, argc - 1, argv + 1);

      if(e == -1)
         {
         std::cout << "Unknown command " << cmd << "\n";
         return help(argc, argv);
         }

      return e;
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
