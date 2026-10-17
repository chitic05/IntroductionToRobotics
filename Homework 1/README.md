#Exercitiul 1

Componente: -o baterie de 5V
	    -N rezistori, N>=2
            -un LED rosu
			
Calcule: 
	Am dorit sa obtin o intensitate de 20mA, asadar suma rezistentelor + rezistenta LED-ului trebuie sa fie 250ohmi. La 20mA LED-ul are o caderea de tensiune de 1.85V, deci rezistenta acestuia este de 92.5 ohmi.Alegem cele N rezistente astfel incat suma rezistentelor trebuie sa fie 157.5 ohmi (rezistentele sunt in serie). 
	Caderea de tensiune a fiecarui rezistor este (Rezistenta acestuia) * (20mA) V. 
	Puterea disipita de fiecare rezistor este (Caderea de tensiune a acestuia) * (Intensitatea curentului) W.
	Verificare KVL:
	V sursa = V rezistori + V led
	5V = 2*(78.8ohmi * 0.02A) + 1.8V <=> 5V = 4.952V ADEVARAT (aproximativ)

#Exercitiul 2

Componente: -o baterie de 9V
-2 rezistori( 1450ohmi, respectiv 357.5 ohmi)
-2 LED-uri rosii

Calcule:
	Doresc pe primul branch sa obtin 5mA si pe al doilea branch sa obtin 20mA. Din KCL I total = I1+I2 (I1= 5mA si I2= 20mA), asadar I total =  25mA.
	V total = 9V, I total = 25mA => R total = 360ohmi.
	Circuitul este in paralel, deci 1/R total = 1/R1 + 1/R2.
	R1 = R rezistor1 (1450 ohmi) + R LED1(350 ohmi), R2 = R rezistor2 (357.5 ohmi) + R LED2(92.5 ohmi)
	Diferenta de rezistenta dintre LED-uri e cauzata de diferenta curentului de pe cele doua branch-uri.
	R1 = 1800 ohmi si R2 = 450 ohmi.
	Tensiunea fiind aceiasi pe fiecare ramura => P rezistor1 = (0.005A * 1450 ohmi) * 0.005A = 0.03625 W
	P rezistor2 = (0.02A * 357.5 ohmi) * 0.02A = 0.143 W
	In fiecare ochi se respecta KVL:
		Ochiul 1: V in = V rezistor1 + V LED1 <=> 9V = 7.25V (5mA*1450ohmi) + 1.75V = 9V ADEVARAT
		Ochiul 2: V in = V rezistor2 + V LED2 <=> 9V = 7.15V (20mA * 357.5) + 1.85V = 9V ADEVARAT

#Exercitiul 3

Componente: -o baterie de 12V
	    -2 rezistori de 600 ohmi
	    -1 LED rosu
	    -un buton
	    -2 condensatori cu o capacitate de 5 uF

Condensatori in serie:
	-Capacitatea totala scade (1/Ctotala = 1/C1 + 1/C2)
	-Sarcina electrica este egala in toti condensatorii
	-Se incarca mai rapid decat cei pusi in paralel, dar se si descarca mult mai rapid
	-C totala = (C1*C2)/(C1+C2) = 25/10 uF = 2.5 uF
	-tau_incarcare = R * C_totala = 600 * 2.5 * 10^(-6) = 0.0015s
	-t_incarcare_6V = -tau_incarcare * ln(1 - V_tinta/V_max) = -0.0015 * ln(0.5) = 0.001039s
	-tau_descarcare = (R1 + R2) * C_totala = 1200 * 2.5 * 10^(-6) = 0.003s
	-t_descarcare_de_la_6V_la_1.5V = -tau_descarcare * ln(V_tinta/V_curent) = -0.003 * ln(1.5V/6V) = 0.004158s

Condensatori in paralel:
	-Capacitatea totala creste (Ctotala = C1+C2)
	-Sarcina electrica se aduna intre condensatori (Qtotala = Q1+Q2)
	-Se incarca mai lent decat cei pusi in serie, dar se descarca mult mai lent
	-C totala = 10uF
	-tau_incarcare = R * C_totala = 600 * 10 * 10^(-6) = 0.006s
	-t_incarcare_6V = -tau_incarcare * ln(1 - V_tinta/V_max) = -0.006 * ln(0.5) = 0.004158s
	-tau_descarcare = (R1 + R2) * C_totala = 1200 * 10 * 10^(-6) = 0.012s
	-t_descarcare_de_la_6V_la_1.5V = -tau_descarcare * ln(V_tinta/V_curent) = -0.012 * ln(1.5V/6V) = 0.01663s
