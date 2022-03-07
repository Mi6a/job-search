#include <iostream>
#include <string>
#include <memory>
#include "include/cpp20/unordered_set"
#include "include/cpp20/unordered_map"

#include "OrderCacheImpl.h"

using namespace std::string_literals;

using std::cout; using std::cin; using std::endl;

using StrShared = std::shared_ptr<std::string>;

struct StrSharedEqual {
   using is_transparent = void; // for find() override

   bool operator()(const StrShared& ss1, const StrShared& ss2) const  noexcept {
      return *ss1 == *ss2;
   }
   bool operator()(const std::string& s1, const StrShared& ss2) const  noexcept {
      return s1 == *ss2;
   }
   bool operator()(const StrShared& ss1, const std::string& s2) const  noexcept {
      return *ss1 == s2;
   }
};

//
struct StrSharedHash {
   using is_transparent = void; // for find() override

   std::size_t operator()(const StrShared& ss) const noexcept {
      return std::hash<std::string>{}(*ss); 
   }

   std::size_t operator()(const std::string& s) const noexcept {
      return std::hash<std::string>{}(s);
   }
};

using PtrStrSet = std::unordered_set<StrShared, StrSharedHash, StrSharedEqual>;

using PtrStrIntMap = std::unordered_map<StrShared, int, StrSharedHash, StrSharedEqual>;

//
int test()
{
   std::string s1 = "Serg"s;
   auto ss1 = std::make_shared<std::string>(s1);
   // testing set with cxx20
   PtrStrSet pss;
   auto resS = pss.insert(ss1);
   cout << "Inserted: " << (resS.second? "yes" : "no") << endl;
   auto foundS = pss.find(s1);
   cout << "Found: ";
   if (foundS != pss.end())
      cout << "yes: " << **foundS << endl;
   else
      cout << "no" << endl;

   // testing map with cxx20
   PtrStrIntMap msi1;
   auto resM = msi1.insert({ss1, 2});
   cout << "Inserted: " << (resM.second ? "yes" : "no") << endl;
   auto foundM = msi1.find(s1);
   cout << "Found: ";
   if (foundM != msi1.end())
      cout << "yes: " << (*foundM).second << endl;
   else
      cout << "no" << endl;

   return 0;
}

int main() {
   OrderCacheImpl orders;
}