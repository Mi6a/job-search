#pragma once

#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "OrderCache.h"

// Implementation selection explanation:
//   1. The decisions were made toward maximizing the speed at the memory expense
//   2. Orders are kept in the unordered_map
//   3. In addition to make search faster, we keep 2 additional hashes, 
//      indexed by m_user and m_securityId
//   4. To make search/compare even more faster and to avoid strings duplication,
//      we keep all strings in single instance and all structures will use shared_ptr<string>
//   5. There're certain improvements, that were not implemented due to time and C++ version constraints:
//       - optimizing multithreading;
//       - improving memory management for faster deletion/allocation, etc.
//       hopefully those can be discussed during the further interview steps

// ASSUMPTIONS:
// UserIds are unique througout the cache, not per the company

//
class OrderCacheImpl : public OrderCacheInterface {
public:
   // add order to the cache
   virtual void addOrder(Order order);

   // remove order with this unique order id from the cache
   virtual void cancelOrder(const std::string& orderId);

   // remove all orders in the cache for this user
   virtual void cancelOrdersForUser(const std::string& user);

   // remove all orders in the cache for this security with qty >= minQty
   virtual void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty);

   // return the total qty that can match for the security id
   virtual unsigned int getMatchingSizeForSecurity(const std::string& securityId);

   // return all orders in cache in a vector
   virtual std::vector<Order> getAllOrders() const;

private:

   using StrShared = std::shared_ptr<std::string>;

   void cancelOrder(StrShared orderId);

   // there're additional members which are to be used with cxx20 find() template
   struct StrSharedEqual {
      using is_transparent = void;  

      bool operator()(const StrShared& ss1, const StrShared& ss2) const  noexcept {
         return ss1.get() == ss2.get();
      }
      
      bool operator()(const std::string& s1, const StrShared& ss2) const  noexcept {
         return s1 == *ss2;
      }
      
      bool operator()(const StrShared& ss1, const std::string& s2) const  noexcept {
         return *ss1 == s2;
      }
   };

   // there're additional members which are to be used with cxx20 find() template
   struct StrSharedHash {
      using is_transparent = void; 

      std::size_t operator()(const StrShared& ss) const noexcept {
         return std::hash<std::string>{}(*ss);
      }

      std::size_t operator()(const std::string& s) const noexcept {
         return std::hash<std::string>{}(s);
      }
   };

   using StrSharedSet = std::unordered_set<StrShared, StrSharedHash, StrSharedEqual>; 

   //
   struct OrderImpl {
      StrShared m_id;
      StrShared m_user;
      StrShared m_comp;
      StrShared m_sec;
      bool      m_side;  // true - "Buy"
      unsigned  m_qty;
   };

   using OrderShared = std::shared_ptr<OrderImpl>;

   using StrOrderMap = std::unordered_map<StrShared, OrderShared, StrSharedHash, StrSharedEqual>;

   // there're additional members which are to be used with cxx20 find() template
   struct OrderSharedEqual {
      using is_transparent = void;

      bool operator()(const OrderShared& o1, const OrderShared& o2) const  noexcept {
         return o1.get() == o2.get();
      }
      
      bool operator()(const std::string& s1, const OrderShared& o2) const  noexcept {
         return s1 == *(o2->m_id);
      }
      
      bool operator()(const OrderShared& o1, const std::string& s2) const  noexcept {
         return *(o1->m_id) == s2;
      }
   };

   // there're additional members which are to be used with cxx20 find() template
   struct OrderSharedHash {
      using is_transparent = void;

      std::size_t operator()(const OrderShared& o) const noexcept {
         return std::hash<std::string>{}(*(o->m_id));
      }

      std::size_t operator()(const std::string& s) const noexcept {
         return std::hash<std::string>{}(s);
      }
   };

   using OrderSet = std::unordered_set<OrderShared, OrderSharedHash, OrderSharedEqual>;

   struct User {
      StrShared _comp;
      OrderSet  _orders;
   };

   using StrUserMap = std::unordered_map<StrShared, User, StrSharedHash, StrSharedEqual>;

   using SecOrdersMap = std::unordered_map<StrShared, OrderSet, StrSharedHash, StrSharedEqual>;

   StrOrderMap    m_orders;
   StrSharedSet   m_companies;
   StrUserMap     m_users;
   SecOrdersMap   m_securs;
};