# Low Level Programming of RPI in C

## C's pointers, addresses and dereferencing

- A *pointer* is a variable that holds a memory address. A pointer is declared using the `*` operator.
- A memory *address* of a variable can be obtained using the `&` operator. This is called "address-of" operator.
```c
int var = 42;
int *ptr = &var; // Declare a pointer variable and assign it the address of 'var'.
```

- Dereferencing a pointer on the **right** side, returns the **value** stored at its memory *address*.
```c
int value = *ptr;
printf("Value: %d\n", value); // Output: Value: 42
```

- Dereferencing a pointer on the **left** side, stores a **new value** at its memory *address*.
```c
*(ptr) = 100;
printf("Value: %d\n", var); // Output: Value: 100
```

- Dereferencing a *pointer to a pointer* on the **right** side, returns the **value** stored at the memory *address* that the *pointer to pointer* points to.
```c
int **ptr_to_ptr = &ptr;
int value = **ptr_to_ptr;
printf("Value: %d\n", value); // Output: Value: 100
```

### Arrays and pointers

In C, these two declarations are equivalent:
```c
arr[i] = value;
*(arr + i) = value;
```

And these two assignments are also equivalent:
```c
int value = arr[i];
int value = *(arr + i);
```

### Structs and pointers

In C, these two references to a struct member are equivalent:
```c
p->member = value;
(*p).member = value;
```

And these two references to a struct member are also equivalent:
```c
int value = p->member;
int value = (*p).member;
```

## Understanding PJ's code

PJ's github: [Pieter-Jan/PJ_RPI](https://github.com/Pieter-Jan/PJ_RPI)

### IO Access

The `bcm2835_peripheral` struct encapsulates the necessary information to access a peripheral's registers in memory.
```c
struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};
```

- `addr_p` is the **physical** address of the peripheral.
- `mem_fd` is the **virtual** file descriptor for the memory device.
- `map` is a pointer to the **virtual** memory mapping of the peripheral.
- `addr` is a pointer to the specific register within the **virtual** memory mapping.

### Memory Mapping

The `mmap` function is used to create a virtual memory mapping of the physical memory where the peripheral is located. This allows us to access the peripheral's registers as if they were regular memory addresses in our program.
```c
p->map = mmap(
  NULL,
  BLOCK_SIZE,
  PROT_READ|PROT_WRITE,
  MAP_SHARED,
  p->mem_fd,  // File descriptor to physical memory virtual file '/dev/mem'
  p->addr_p   // Address in physical map that we want this memory block to expose
);
```

### Accessing Registers

The `addr` pointer is cast to a pointer of type `volatile unsigned int *`, allowing direct access to the peripheral’s registers as **32-bit** values. `addr[0]` would access the first register, `addr[1]` the second, and so on.
```c
p->addr = (volatile unsigned int *)p->map;
```

### Priority

`memset` initializes the `sched` struct to zero.
```c
memset (&sched, 0, sizeof(sched));
```

The `priorityLevel` is set to the maximum priority level for the `SCHED_RR` scheduling policy.
```c
if (priorityLevel > sched_get_priority_max (SCHED_RR))
  priorityLevel = sched_get_priority_max (SCHED_RR);
sched.sched_priority = priorityLevel;
```

`sched_setscheduler` is called to set the scheduling policy and priority for the current process (PID 0).
```c
return sched_setscheduler (0, SCHED_RR, &sched);
```

### GPIO setup macros

- All GPIO pins are grouped in 32-bit registers.
- Every register holds 10 pins.
- Every pin has 3 bits.
- Register bits are numbered **0 - 31** from **right to left**.
  - `[31][30][29][28]...[3][2][1][0]`
  - `reg_addr = 1 << n` sets the nth bit in the register to `1`.

#### Input mode

```c
#define INP_GPIO(g) 	*(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
```

- `g / 10` is the register number for pin `g`.
- `gpio.addr + ((g)/10)` is the address of the register for pin `g`.
- `*(gpio.addr + ((g)/10))` dereferences the register address, allowing us to read or modify the register's value.

```c
register_number = g / 10;
register_address = gpio.addr + register_number;
register_value = *(gpio.addr + register_number);
```

- `g % 10` is the pin number within the register for pin `g`.
- `(g % 10) * 3` is the bit position for pin `g` within the register.

```c
pin_number = g % 10; // in its register
pin_bit_position = (g % 10) * 3; // in its register
```

- `7` is `0b111` in binary
- `7 << ((g % 10) * 3)` masks the 3 bits for pin `g` in the register.
- `&= ~(7 << ((g % 10) * 3))` clears the 3 bits for pin `g` in the register, setting it to input mode.
  - `b & 0 = 0`: AND with `0` clears the bits.
  - `b & 1 = b`: AND with `1` leaves the bits unchanged.

```c
mask = 7 << ((g % 10) * 3);
register_value &= ~mask; // clear the 3 bits for pin g
*(gpio.addr + ((g)/10)) = register_value;
```

#### Output mode

```c
#define OUT_GPIO(g) 	*(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
```

- `1 << ((g % 10) * 3)` sets the first bit for pin `g` in the register, configuring it as output mode.
  - `b | 0 = b`: OR with `0` leaves the bits unchanged.
  - `b | 1 = 1`: OR with `1` sets the bits to `1`.

```c
mask = 1 << ((g % 10) * 3);
register_value |= mask; // set the first bit for pin g
*(gpio.addr + ((g)/10)) = register_value;
```

#### Alternate function mode

```c
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))
```

- `a <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2` determines the value to set for the alternate function based on the value of `a`.
  - If `a` is `0`, `1`, `2`, or `3`, it sets the bits to `a + 4` (which corresponds to alternate functions 4, 5, 6, or 7).
  - If `a` is `4`, it sets the bits to `3` (which corresponds to alternate function 3).
  - If `a` is greater than `4`, it sets the bits to `2` (which corresponds to alternate function 2).

```c
if (a <= 3) {
  alt_value = a + 4; // alternate function 4, 5, 6, or 7
} else if (a == 4) {
  alt_value = 3; // alternate function 3
} else {
  alt_value = 2; // alternate function 2
}
mask = alt_value << ((g % 10) * 3);
register_value |= mask; // set the bits for pin g to the alternate function value
*(gpio.addr + ((g)/10)) = register_value;
```

### GPIO operations macros

#### GPIO set

```c
#define GPIO_SET 	*(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
GPIO_SET = 1 << 17;
```

- `gpio.addr + 7` is the address of the **GPIO set register**.
- `1 << 17` sets the bit for GPIO pin 17, turning it on.

#### GPIO clear

```c
#define GPIO_CLR 	*(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0
GPIO_CLR = 1 << 17;
```

- `gpio.addr + 10` is the address of the **GPIO clear register**.
- `1 << 17` sets the bit for GPIO pin 17, turning it off.

#### GPIO read

```c
#define GPIO_READ(g) 	*(gpio.addr + 13) &= (1<<(g))
int value = GPIO_READ(17) >> 17;
```
- `gpio.addr + 13` is the address of the **GPIO level register**.
- `1 << g` creates a mask for pin `g`.
- `&= (1 << g)` masks the level register to isolate the bit for pin `g`.
- `>> 17` shifts the result to the right by 17 bits, which gives `1` if pin 17 is high, `0` if pin 17 is low.

