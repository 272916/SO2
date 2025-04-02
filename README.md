# Systemy Operacyjne 2 -- Projekt: Problem jedzących filozofów
## Wymagania
- wykorzystanie wielowątkowości
- wykorzystanie mechanizmów zabezpieczających sekcje krytyczne: mutexy, semafory
- bez użycia modułów z gotową synchronizacją
- kod i komentarze w języku angielskim
- każdy z wątków raportuje o stanie, w którym się znajduje (wystarczy wydruk w konsoli)
- nigdy nie powinno dojść do trwałego zablokowania wątków
- program jako argument otrzymuje liczbę filozofów

## Uruchomienie projektu
Przed próbą uruchomienia programu należy upewnić się, że ma się zainstalowany kompilator C++ oraz CMake.
Program oryginalnie napisany został na systemie Windows 11 przy użyciu MinGW.

Kroki zbudowania i uruchomienia projektu są następujące:

### 1. Sklonowanie repozytorium:
```
git clone https://github.com/272916/SO2.git
```
```
cd SO2
```

### 2. Stworzenie i wejście do katalogu build
```
mkdir build
```
```
cd build
```
katalog build powinien być na tym samym poziomie, co katalog /src

### 3. Konfiguracja plików potrzebnych do zbudowania projektu narzędziem CMake i utworzenie pliku wykonywalnego, zależnie od używanego kompilatora
MinGW:
```
cmake -G "MinGW Makefiles" ..
```
```
mingw32-make
```

Clang:
```
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```
```
make
```

MSVC:
```
cmake -G "Visual Studio 17 2022" ..
```
```
cmake --build
```

GCC:
```
cmake -DCMAKE_CXX_COMPILER=g++ ..
```
```
make
```

plik wykonywalny o nazwie SO2 powinien zostać utworzony w katalogu /build

### 4. Uruchomienie programu
```
./SO2 X
```
X należy zamienić na liczbę całkowitą będącą liczbą liczbą filozofów w programie. Zbyt duża liczba może sprawić, że aktualne stany filozofów wyświetlane w konsoli będą nieczytelne

## Opis problemu
Problem jedzących filozofów jest problemem synchronizacji procesów w programowaniu współbieżnym, sformułowanym oryginalnie w 1965r. przez Edsgera Dijkstrę.

Filozofowie siedzą przy jednym okrągłym stole. Każdy z nich wykonuje jedną z dwóch czynności: myśli, lub je.

Pomiędzy każdą parą filozofów znajduje widelec (w tym wariancie problemu). Widelców łącznie jest tyle samo, co filozofów.

Dany filozof może jeść tylko i wyłącznie, gdy posiada dwa widelce. Nie może jednak sięgnąć po widelec, który nie znajduje się bezpośrednio obok niego.

Problemem jest zapewnienie, aby żadnej filozof nie głodował przez zbyt długi okres czasu.

Przykładowo: Wszyscy filozofowie mogą podejść jednocześnie do stołu i wziąć po jednym widelcu. Gdyby wszyscy następnie czekali, aż zwolni się drugi widelec, to doszłoby do zakleszczenia i wszyscy pozostaliby głodni.

W odniesieniu do programowania współbieżnego:
- filozofowie odpowiadają wątkom
- widelce odpowiadają sekcji krytycznej (wspólnym zasobom)
- jedzenie odpowiada wykonywaniu operacji wykorzystujących wspólne zasoby

## Opis rozwiązania

Do symulacji oraz rozwiązania problemu, program wykorzystuje trzy klasy:

### Fork
Klasa Fork reprezentuje widelce potrzebne filozofom do jedzenia.

Wątek filozofa wchodzi do sekcji krytycznej, gdy podnosi lub odkłada widelec.
Aby jeden widelec nie mógł zostać podniesiony przez więcej niż jednego filozofa, każdy obiekt klasy Fork zachowuje się jak mutex dla danego widelca. Metoda *Take* do podnoszenia widelca zapętla się i blokuje wątek filozofa, dopóki widelec nie będzie dostępny.

### Table
Klasa Table reprezentuje stół, do którego podchodzą filozofowie, aby podnieść widelce i jeść.

Chociaż oryginalne sformułowanie problemu się na tym nie skupia, w implementacji tego projektu stół jest kolejną sekcją krytyczną. Bez żadnej blokady lub synchronizacji, paru filozofów mogłoby podejść do stołu w tym samym momencie i obejść jego limit miejsc. Stół wykorzystuje zatem jako mutex obiekt klasy Fork, służący do blokowania modyfikacji miejsc.

Limit miejsc stołu ustawiony jest na liczbę o 1 mniejszą od liczby filozofów. Gdyby miejsc przy stole było tyle samo, co filozofów, to mogłoby dojść do zakleszczenia (ang. deadlock) w postaci wzięcia jednego widelca przez każdego z filozofów. Jeżeli filozofów przy stole jest mniej niż widelców, to co najmniej jeden filozof będzie w stanie podnieść dwa widelce, zjeść, a następnie odejść do stołu i zwolnić oba widelce.

O ile mechanizm jej działania nie jest tradycyjny (zwiększa licznik zużytych zasobów aż do limitu, zamiast zmniejszać licznik zasobów dostępnych), to stół zasadniczo emuluje działanie semaforu, kontrolując dostęp do zasobów wspólnych (widelców).

### Philosopher
Klasa Philosopher reprezentuje filozofów. Pole *phil_thread* jest wątkiem odpowiadającym za zachowanie danego filozofa.

Wątki filozofów synchronizowane są przez obiekty klas Fork i Table.

Zachowanie wątku wygląda następująco:
- filozof myśli przez określony dla niego czasu (*time_thinking*)
- filozof przechodzi w stan głodu i podchodzi do stołu
- filozof czeka, aż będzie możliwe podejście do stołu (przy stole będzie co najmniej jedno wolne miejsce, żadnej inny filozof aktualnie nie podchodzi do/odchodzi od stołu)
- filozof podchodzi do stołu i próbuje podnieść oba widelce obok jego miejsca
- filozof czeka, aż oba widelce przy jego miejscu będą wolne
- filozof je przez określony dla niego czas (*time_eating*)
- filozof odkłada oba widelce, zwalniając oba zasoby
- filozof odchodzi od stołu, zwalniając zasób w postaci miejsca przy stole

Aktualny stan filozofa (zamyślony, głodny, jedzący) zmieniany jest na bierząco wraz z działaniem wątka danego filozofa.

Wypisywanie własnego stanu przez każdego filozofa przy każdej zmianie stanu było bardzo nieczytelne i powodowało co jakiś czas *race condition*, gdy więcej niż jeden filozof próbował wypisać swój stan do konsoli jednocześnie, mieszając ze sobą komunikaty. Wprowadzony został mechanizm synchronizacji poprzez blokowanie pojedynczego zasobu pozwalającego na pisanie do konsoli. To rozwiązanie jednak mogło bardzo łatwo powodować pewnego rodzaju zagłodzenie, w którym któryś filozof nigdy nie komunikowałby swojego stanu (pomimo jego zmiany).

Aktualna wersja programu wykorzystuje pojedynczy osobny wątek do wypisania stanu każdego filozofa po kolei co 500ms.
Rozwiązanie to znacząco poprawia czytelność, chociaż nie jest idealne. Wewnętrzne stany filozofów w zmieniają się ciągle, a wyświetlane są tylko co 500ms. Oznacza to, że zmiany mogą zostać pominięte podczas wyświetlania, jeżeli staną się zbyt szybko. Sposób ten zdaje się jednak być lepszy od poprzedniego.