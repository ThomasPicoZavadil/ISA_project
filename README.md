# ISA projekt - DNS resolver

**Author:** Tomáš Zavadil  
**Login:** xzavadt00  
**Date:** 17. 11. 2025

---

## Popis programu

Tento program implementuje **DNS proxy**, která přijímá DNS dotazy typu **A**, filtruje je podle dodaného seznamu zakázaných domén a subdomén a poté je přeposílá na upstream DNS server. Očekává odpověď od upstream resolveru a následně ji vrací zpět klientovi.

---

## Rozšíření

- Podpora argumentu `-v` pro průběžné vypisování informací (verbose mód).

---

## Omezení

- Aplikace zpracovává DNS dotazy **pouze sekvenčně** (jeden po druhém).  
  Paralelní / asynchronní zpracování není implementováno.
---

## Příklad spuštění

### 📦 Přeložení a ruční spuštění

```sh
make
./dns -s 8.8.8.8 -p 4444 -f filter_file.txt
```

### Přeložení a automatické spuštění
```sh
make run
```

### Přeložení a spuštění automatizovaných testů
```sh
make test
```

Seznam odevzdaných souborů:

/src
    main.c
    args.c
    args.h
    dns.c
    dns.h
    filter.c
    filter.h
    forwarder.c
    forwarder.h
makefile
test_dns.sh
README.md