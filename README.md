# Telemetry-Project-Operating-Systems
 
 Scrieti un daemon in userspace, insotit de o librarie care sa expuna functionalitatea acestuia, care sa implementeze distributia de mesaje de lungime scurta, organizate pe canale de distributie, intre mai multi participanti.
 
 Canalele de distributie vor fi organzizate ierarhic, de asa natura incat, un mesaj trimis pe un canal parinte, va fi receptionat de toti cei inregistrati la canalul respectiv impreuna cu toti aceia inregistrati la canalele copil ale acestuia. Canalele parinte vor fi create automat in momentul in care daemonul semnaleaza inregistrarea a cel putin unui participant la comunicare pe un canal copil.
 
 Exista doua tipuri de participanti: cei care trimit mesajele, numiti "publishers" si cei care le receptioneaza, numiti "subscribers".
 
## Exemplu:
   Un set de canale posibile la un moment dat, ar putea fi:
     A: /comm/messaging/channel_a
     B: /comm/messaging/channel_b
     C: /comm/messaging
     D: /comm
   Pentru acest exemplu, avem urmatoarele afirmat, ii corecte:
     1. Un mesaj trimis catre canalul (A) va fi receptionat doar de (A)
     2. Un mesaj trimis catre (C) va fi receptionat de (A) si (B)
     3. Un mesaj trimis catre (D) va fi receptionat de (A),(B),(C) si (D).
     4. Cand canalul (A) este creat, automat vor fi disponibile canalele parinte (C) si (D)
