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
// h()
// -> 1 # This should work. #
// f(uint256): 1, 2
// -> FAILURE
