# EclairLang

## syntax


* ecl - defenition of variable
example:
```
ecl x : int = 1;
```


* fn - defenition of function
example:
```
fn factorial(n: int): int {
    if (n < 2) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

