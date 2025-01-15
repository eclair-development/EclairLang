fn sieve_of_eratosthenes(limit: int) {
    ecl is_prime: int[100000];
    eclor ecl i: int = 0; i < limit; i++ {
        is_prime[i] = 1;
    }
    is_prime[0] = 0;
    is_prime[1] = 0;

    eclor ecl p: int = 2; p * p <= limit; p++ {
        eclif (is_prime[p] == 0) {
            continue;
        }
        eclor ecl multiple: int = p * p; multiple <= limit; multiple = multiple + p {
            is_prime[multiple] = 0;
        }
    }

    eclor ecl i: int = 2; i <= limit; i++ {
        eclif (is_prime[i]) {
            eclay(i);
            eclamaticPause();
        }
    }
}

fn main(): float {
 sieve_of_eratosthenes(100);
 eclout 0;
}
