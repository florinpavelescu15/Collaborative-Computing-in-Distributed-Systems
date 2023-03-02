# Pavelescu Florin, 334CC
##ALGORITMI PARALELI SI DISTRIBUITI
##Tema #3
##Calcule colaborative in sisteme distribuite
#PREAMBUL
Rezolvarea temei mi-a luat aproximativ 6 ore. Tema mi s-a parut foarte 
interesanta si accesibila, fiind foarte asemanatoare cu cerintele din
cadrul laboratoarelor. Am urmat intocmai indicatiile din cerinta privind
numarul de procese si schimbul de mesaje intre acestea.
#DETALII DE IMPLEMENTARE
##Conventii: 
- Pentru procesul curent de rang k, procesul urmator e procesul de rang
`k + 1`, iar procesul anterior/precedent e procesul de rang `k - 1`;
- `i->j` = clusterul i trimite un mesaj clusterului `j` si clusterul `j`
primeste un mesaj de la clusterul `i`; 
- `i[->workeri_subordonati]->j` = clusterul `i` trimite un mesaj workerilor
subordonati lui si clusterului `j` si acestia il primesc;
- `[workeri_subordonati->]i->j` = clusterul `i` primeste cate un mesaj de
la fiecare worker subordonat lui si trimite un mesaj clusterului `j` si
acesta il primeste.

Am tratat doua cazuri:
1. Legatura circulara intre clustere `(0->1->2->3->0)`, corespunzator
primelor doua cerinte;
2. Legatura intrerupta intre clusterele `0` si `1` `(0->3->2->1->2->3->0)`,
corespunzator cerintei 3.

##Cazul 1.
- Stabilirea topologiei:
In clusterul `0` preiau numarul total de procese din functia `MPI_Comm_size()`,
citesc din fisierul cluster.txt si retin datele in vectorii `topology` si
`workers` `(topology[i] = k` <=> procesul i este un worker subordonat clusterului
`k` si `workers[i] = n` <=> clusterul `i` are `n` workeri subordonati). Trimit aceste
informatii procesului urmator. Clusterele `1` si `2` primesc datele de la 
clusterul anterior, adauga in vectori informatiile citite din fisier si
trimit vectorii la clusterul urmator. Clusterul `3` primeste informatiile de la
clusterul anterior si adauga in vectori informatiile citite din fisier.
Acum topologia este completa, iar clusterul `3` trimite topologia completa
la clusterul `0`. Clusterul 0 trimite topologia workerilor subordinati lui si
apoi o trimite din aproape in aproape pana la clusterul `3`. Fiecare cluster, 
mai putin cluster `0`, primeste topologia de la clusterul anterior, o afiseaza
si o trimite mai departe la clusterul urmator (mai putin clusterul `3`) si la
workerii subordonati, care o primesc si o afiseaza.
###Rezumat:
`0->1->2->3->0[->workeri_subordonati0]->1[->workeri_subordonati1]->
           ->2[->workeri_subordonati2]->3[->workeri_subordonati3]`

- Realizarea calculelor:
Dupa stabilirea topologiei, fiecare worker isi cunoaste clusterul coordonator.
Clusterul `0` genereaza `v` si il trimite, din aproape in aproape, pana la
clusterul `3`. Fiecare cluster imediat ce primeste vectorul `v`, il trimite la
la clusterul urmator (mai putin cluster `3`) si la workerii subordonati.
Fiecare worker primeste vectorul, ia elementele cuprinse intre start
si end, le inmulteste cu 5 si le salveaza in vectorul `res` de dimensiune
`size = end - start`. Valorile `start` si `end` au fost calculate astfel incat
calculele sa fie echilibrate in raport cu workerii, folosind formulele 
din laborator (`N` e dimensiunea vectorului si `rank` e rangul procesului):
    `start = (rank - 4) * N / (NUMBER_OF_PROCESES - 4)`
    `end = min((rank - 3) * N / (NUMBER_OF_PROCESES - 4), N)`
Am folosit `rank - 4`, intrucat sunt `NUMBER_OF_PROCESES - 4` procese worker
indexate de la `4` la `NUMBER_OF_PROCESES` (procesele `0 - 3` sunt clustere).
Imediat ce calculele au fost efectuate, fiecare worker trimite res, start
si end la clusterul coordonator. Fiecare cluster, incepand cu cluster `3`,
primeste vectorii `res` de la workerii subordonati, adauga in vectorul `v`
valorile din vectorii `res` pe baza valorilor `start` si `end` primite si trimite
vectorul `v` cu modificarile precizate, din aproape in aproape, pana la cluster `0`,
care afiseaza vectorul modificat complet.
###Rezumat:
`(0[->workeri_subordonati0]->1[->workeri_subordonati1]->
->2[->workeri_subordonati2]->3[->workeri_subordonati3] calcule
[workeri_subordonati3->]3->[workeri_subordonati2->]2->
->[workeri_subordonati1->]1->[workeri_subordonati0->]0 final`

##Cazul 2. 
Foarte asemanator cazului anterior. Voi prezenta doar rezumatele
atat pentru partea de stabilire a topologiei cat si pentru cea de realizare
a calculelor.
- Stabilirea topologiei:
`0->3->2->1[->workeri_subordonati1]->2[->workeri_subordonati2]->
       ->3[->workeri_subordonati3]->0[->workeri_subordonati0]`

- Realizarea calculelor:
`0[->workeri_subordonati0]->3[->workeri_subordonati1]->
->2[->workeri_subordonati2]->1[->workeri_subordonati3] calcule
[workeri_subordonati1->]1->[workeri_subordonati2->]2->
->[workeri_subordonati1->]1->[workeri_subordonati0->]0 final`

Mai multe detalii despre implementare se gasesc in comentariile din cod.
