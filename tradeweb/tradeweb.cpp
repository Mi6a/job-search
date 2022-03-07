#include <iostream>
#include <string>
#include <memory>
#include "include/cpp20/unordered_set"

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

//
int main()
{
   PtrStrSet pss;
   std::string s1 = "Serg"s;
   auto ss1 = std::make_shared<std::string>(s1);
   auto res = pss.insert(ss1);
   cout << "Inserted: " << (res.second? "yes" : "no") << endl;
   auto found = pss.find(s1);
   cout << "Found: ";
   if (found != pss.end())
      cout << "yes: " << **found << endl;
   else
      cout << "no" << endl;
   return 0;
}
