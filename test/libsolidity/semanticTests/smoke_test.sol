contract C {
    function f() public returns (uint) {
        return 1;
    }
    function g(uint x, uint y) public returns (uint) {
        return y - x;
    }
    function h() public payable returns (uint) {
        return f();
    }
}
// ----
// iam_not_there()
// -> FAILURE
// f()
// -> 1
// g(uint256,uint256): 4, 8
// -> 4
// h(), 314 ether
// -> 1
