# SK2 project
### MQ library
##### *Project for Computer Networks course on fifth semester on Poznan University of Technology*

**Description (in polish):**
Message queue – message broker (serwer), biblioteka klienta i demonstracyjny program używający tej biblioteki. Klient może tworzyć kolejki, wysyłać do nich wiadomości i zapisywać się do odbioru wiadomości z kolejki. Kolejki wspierają wielu konsumentów, wiadomości mają określony czas ważności.

**Exemplary usage:**
- Server
> ./server {port}
- Client
> ./client {ip} {port} {producent|consument} {queue no.} [delay beetwen messages /*producent*/]

**Authors:**
- Patryk Jedlikowski
- Maciej Leszczyk
