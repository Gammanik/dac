#include "../../include/dac/dac.staker.hpp"


namespace eosio {

using std::string;

void staker::handle_transfer( name from, name to, asset quantity, std::string memo ) {
    if (from == _self) return;
    // EOSBet hack
    eosio_assert(to == _self, "transfer must be direct to contract.");
    // Check the buy code is valid
    const string transfer_code = memo.substr(0, 7);
    
    eosio_assert(transfer_code == "staking" || transfer_code == "", "malformed memo string");
    if (transfer_code == "") { return; }
    
    if (transfer_code == "staking") {
      eosio_assert(quantity.symbol == symbol("EOS", 4), "you must transfer EOS tokens for staking");
      eosio_assert( quantity.is_valid(), "quantity is not valid." );
      handle_staking( quantity );
    }
    
   
}


void staker::handle_staking( asset quantity ) {
  dapps _dapps( VOTER_CONTRACT, VOTER_CONTRACT.value);
  
  // todo: add total_staked field to the voting contract
  // for(auto itr = _dapps.begin(); itr != _dapps.end();) {
  //     itr = _dapps.erase(itr);
  // }
  
  int size = std::distance(_dapps.cbegin(), _dapps.cend()); // = 0?? why
  // int size = 3; // * 2 because delegated to cpu and ram 50/50
  
  // todo: bug? working 
  asset part = asset( floor(quantity.amount / size*2 )  ,symbol("EOS", 4));
  
  for( const auto& dapp : _dapps ) {
    if ( is_account(dapp.dappname) ) {
      action( permission_level{_self, name("active")},
        name("eosio"), name("delegatebw"),
        std::make_tuple( _self, name(dapp.dappname), part, part, false ))
      .send();
    }
    
    // print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name{item.owner}, "\n");
  }
  
}







// voter::Config voter::_get_config() {
//   Config config;
  
//   if (ConfigSingleton.exists()) {
//     config = ConfigSingleton.get();
//   } else {
//     config = Config{};
//     ConfigSingleton.set(config, _self);
//   }
  
//   return config;
// }





extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    
    // todo: important:: here should be a name of the account with the MG token
    // todo: put it in the Singleton?????
    if (code == name("eosio.token").value && action == name("transfer").value) {
      execute_action(name(receiver), name(code), &staker::handle_transfer);
    }
    
  }
  
}


} // eosio namespace

// EOSIO_ABI( eosio::voter, (eosio::applydapp) )

// EOSIO_DISPATCH(eosio::voter, (eosio::applydapp))


