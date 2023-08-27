# tokenizer
a simple generic code tokenizer written in C, the tokenizer doesn't support specific language,
it will just group words and strings

## build
```sh
make build
```

## example
```py
# test.py

print("hello world")

def foo() -> int:
    return 0
```

output for `tokenizer test.py -gs`
```sh
(NEWLINE)
print
(
"hello world"
)
(NEWLINE)
(NEWLINE)
(NEWLINE)
#
(SPACE)
comment
(NEWLINE)
def
(SPACE)
foo
(
)
(SPACE)
-
>
(SPACE)
int
:
(NEWLINE)
(SPACEx4)
return
(SPACE)
0
```
