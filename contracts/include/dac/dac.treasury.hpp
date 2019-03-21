#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>   /* floor */


namespace eosiosystem {
class system_contract;
}


namespace eosio {

class [[eosio::contract("treasury")]] treasury : public contract {
public:
  using contract::contract;
  void handle_transfer(name from, name to, asset quantity, std::string memo);
  
private:
  
};


}