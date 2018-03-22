# c-xmlc
tool for compiling c programs from .xml files

### example: 
build.xml
```xml
<build>
  <include value="lib"/>
  
  <source value="main.c"/>
  <source value="foo.c"/>
  
  <flag link="m"/>
  <flag output="a.out"/>
  <flag include="src"/>
</build>
```

---> gcc -o a.out -Isrc -lm main.c foo.c
