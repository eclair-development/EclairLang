# The EclairLang


## Syntax

* main function is `main` MUST ECLOUT FLOAT
* eclout - return value from funnction
```
// main function is flood because it returns float 🤡 
fn main(): float {
    eclout 0.0;
}
```

* ecl - defenition of variable
```
ecl x : int = 1;
//by default all arrays values is 0
ecl x : int[1000]
eclay(x[234]) // eclays 0
```

* fn - defenition of function
* eclif - if statement
* eclse - else statement
```
// pleas specifi eclout type in fn decloration
fn factorial(n: int): int {
    eclif (n < 2) {
        eclout 1;
    } eclse {
        eclout n * factorial(n - 1);
    }
}
// if no eclout --> no eclout type specified
fn lnPrintLn(val: int) { 
    eclamaticPause()
    eclay(val)
    eclamaticPause()
}
```

* eclor - equals to for loop
* eclay - to print smth
* eclamaticPause - wait a few seconds, in norators purposes (prints '/n') 
```
eclor ecl i: int = 0; i < 10; i++  {
    eclayInt(i);
    eclamaticPause();
 }
```

* jagermeister - equals to while loop
```
 jagermeister true  {
    eclamaticPause();
 }
```


## JIT-RLA
### what is RLA?

<marquee>In eclair-development we are continuously asking ourselvs what is the purpose of our programming language

we know that any universal tool will be inferior to their task-specific counterparts.

eclair is specific tool for several common usecases
</marquee>

//TODO добавить инфу про оптимизации
### R - recursion
### L - loop
### A - arrays

### How to use JIT?
just add -O opton with optimization u would like to use
```
eclair -ORLA // use all optimizations
eclair -ORA // use recursion and array acsses optimizations
eclair -OL // use only loop optimizations
```
if you Not Sure of what you are doing you can always get help with
```
eclair -h
```  
## Bencmarks

### factorial/fibanachi
```
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
```

run in sandbox directory
```
./eclair -OR fibanaci.ecl 
```

### sieve of eratosthenes
```
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
```

run in sandbox directory
```
./eclair -OLA eratosthenes.ecl
```

### quick sort
```
fn quick_sort(arr: int*, left: int, right: int, n: int) {

    eclif (left >= right) {
        eclout;
    }
    eclayArray(arr, n);
    ecl pivot: int = arr[(left + right) / 2]; // Опорный элемент
    ecl index: int = partition(arr, left, right, pivot);
    quick_sort(arr, left, index - 1, n);
    quick_sort(arr, index, right, n);
}

fn partition(arr: int*, left: int, right: int, pivot: int): int {
    ecl i: int = left;
    ecl j: int = right;

    jagermeister (i <= j) {
        jagermeister (arr[i] < pivot) {
            i++;
        }
        jagermeister (arr[j] > pivot) {
            j--;
        }
        eclif (i <= j) {
            swap(arr, i, j);
            i++;
            j--;
        }
    }
    eclout i;
}

fn swap(arr: int*, i: int, j: int) {
    ecl temp: int = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

fn eclayArray(arr: int*, n: int) {
    eclor ecl i: int = 0; i < n; i++ {
        eclay(arr[i]);
        eclay(" ");
    }
    eclamaticPause();
}

fn main() : float {
    ecl arr: int[10];
    arr[0] = 234;
    arr[1] = 2313;
    arr[2] = 5525;
    arr[3] = 3;
    arr[4] = 13;
    arr[5] = 431;
    arr[6] = 23414;
    arr[7] = 25;
    arr[8] = 1337;
    arr[9] = 1;
    ecl n: int = 10;

    quick_sort(arr, 0, n - 1, n);

    eclout 0;
}
```

run in sandbox directory
```
./eclair -ORLA qsort.ecl
```
