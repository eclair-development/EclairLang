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
