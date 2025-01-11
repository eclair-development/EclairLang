# The EclairLang


## syntax

* main function is `main`
* eclout - return value
```
// main function is float because it returns float
fn main(): float {
    eclout 0;
}
```

* ecl - defenition of variable
```
ecl x : int = 1;
```

* fn - defenition of function
* eclif - if statement
* eclse - else statement
```
fn factorial(n: int): int {
    eclif (n < 2) {
        eclout 1;
    } eclse {
        eclout n * factorial(n - 1);
    }
}
```

* eclor - for loop
* eclayInt - print int
* eclayFloat - print float
* eclamaticPause - print '/n'
```
eclor ecl i: int = 0; i < 10; i++  {
    eclayInt(i);
    eclamaticPause();
 }
```