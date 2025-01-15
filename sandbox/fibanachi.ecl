fn fibanachi(n: int): int {
 eclif (n < 3) {
 eclout 1;
 } eclse {
 eclout fibanachi(n-1) + fibanachi(n-2);
 }
}

fn main(): float {
 eclay(fibanachi(10));
 eclout 0;
}
