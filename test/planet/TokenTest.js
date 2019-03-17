const infeos = require('infeos').init();

const EOSIOApi = infeos.EOSIOApi;
const EOSIORpc = infeos.EOSIOApi.rpc;


describe('Token Contract Tests', function () {

    let eosioTest;
    let account;
    let tokenContractInstance;

    before(async () => {
        eosioTest = new infeos.EOSIOTest();
        account = eosioTest.deployerAccount

        tokenContractInstance = new infeos.EOSIOContract('dac', account);


    })

    it("should create a new token", async () => {
        const maxSuppply = "500000000.0000 MG"
        const issuer = "minergatemgt" // todo: register this name - createaccount

        await tokenContractInstance.create(account.name, maxSuppply, issuer)
        const statTable = await EOSIORpc.get_table_rows({ code: account.name, table: 'stat', scope: "MG"});
        const stat = statTable['rows'];
        const tokenInfo = stat[0];

        assert.isTrue(tokenInfo.max_supply === maxSuppply, `Expect supply to be ${maxSuppply}, but got: ${tokenInfo.supply}`)
        assert.isTrue(tokenInfo.supply === "0.0000 MG", `Expect supply to be 0.000 MG, but got: ${tokenInfo.supply}`)
    })

    it("should issue a token", async () => {

        const to = "minergatemgt" // todo: register this name - createaccount
        const quantity = "100000000.0000 MG"

        await tokenContractInstance.issue(to, quantity, "")
        const statTable = await EOSIORpc.get_table_rows({ code: account.name, table: 'stat', scope: "MG"});
        const stat = statTable['rows'];
        const tokenInfo = stat[0];

        assert.isTrue(tokenInfo.supply === quantity, `Expect supply to be ${quantity}, but got: ${tokenInfo.supply}`)
    })

    it("should airgrab a token at first", async () => {
        const to = "minergatemgt" // todo: register this name - createaccount
        const grabberBalance = "0.4000 EOS"
        const factor = 0.5

        await tokenContractInstance.issue(to, quantity, "")
        const statTable = await EOSIORpc.get_table_rows({ code: account.name, table: 'stat', scope: "MG"});
        const stat = statTable['rows'];
        const tokenInfo = stat[0];
        assert.isTrue(tokenInfo.supply === quantity, `Expect supply to be ${quantity}, but got: ${tokenInfo.supply}`)


        const accountsTable = await EOSIORpc.get_table_rows({ code: account.name, table: 'accounts', scope: to});
        const account = accountsTable['rows'];
        const accountInfo = account[0];
        assert.isTrue(accountInfo.balance === quantity * factor,
            `Expect ${to} balance to be ${grabberBalance}, but got: ${accountInfo.balance}`)


        // cleos get table <CONTRACT_ACCOUNT> <AIRDROPPED_ACCOUNT> accounts
    })

    it("should not take an airgrab if already grabbed", async () => {

    })

})


