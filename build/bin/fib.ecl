//
fn factorial(n: int): int {
 if (n < 2) {
 return 1;
 } else {
 return n * factorial(n - 1);
 }
}

fn fibanachi(n: int): int {
    if (n < 3) {
        return 1;
    } else {
        return fibanachi(n-1) + fibanachi(n-2);
    }
}


fn main(): int {
 let result : int = fibanachi(20);
 eclout result;
}
