# c-xmlc
tool for compiling c programs from .xml files

### example: 
build.xml
```xml
<build compiler="gcc">
    
  <option> -o build/a.out </option>

  <option> -w </option>

  <include> lib/build.xml </include>

  <option include="true" prefix="-I">
    incdir
    libdir
  </option>

  <option include="true">
    main.c
    foo.c
  </option>

  <option prefix="-l">
    openssl
    pthread
  </option>

</build>
```

---> gcc -o build/a.out -Iincdir -Ilibdir main.c foo.c -lopenssl -lpthread
