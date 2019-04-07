#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>
#include <math.h>   /* floor */

namespace eosiosystem {
class system_contract;
}

namespace eosio {
  
typedef uint64_t uuid;
using std::string;

class [[eosio::contract("eosio.staker")]] staker : public contract {
public:
  name VOTER_CONTRACT = name("votertestnet");
  symbol MG_SYMBOL = symbol("MG", 4);
  
  using contract::contract;
  
  void handle_transfer( name from, name to, asset quantity, std::string memo );

private:
  void handle_staking( asset quantity );
  
  // the bable from voting contract
  struct dapps_table {
    uuid  id;
    name      dappname;
    std::string    description;
    asset     votes;
    
    uint64_t primary_key()const { return id; }
  };
  
  typedef eosio::multi_index< "dapps"_n, dapps_table > dapps;
  
};

} /// namespace eosio