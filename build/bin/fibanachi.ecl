
fn factorial(n: int): int {
 eclif (n < 2) {
 eclout 1;
 } eclse {
 eclout n * factorial(n - 1);
 }
}

fn fibanachi(n: int): int {
 eclif (n < 3) {
 eclout 1;
 } eclse {
 eclout fibanachi(n-1) + fibanachi(n-2);
 }
}




fn main(): float {
 eclor ecl i: int = 0; i < 10; i++  {
    eclayInt(i);
    eclamaticPause();
 }
 eclout 0;
}
