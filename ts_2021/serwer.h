#pragma once
#include "protokol.h"

using HOST = std::pair<sf::IpAddress, unsigned short>;

class serwer {
public:
	std::pair<sf::Uint8, std::pair<sf::IpAddress, unsigned short>> host1, host2;
	const unsigned int portSerwera;
	sf::UdpSocket socket;
	bool komunikacja_klientow; // oznacza, ¿e klienci s¹ po³¹czeni

	serwer() : portSerwera(0), komunikacja_klientow(0)
	{
		srand(time(NULL));
		if (socket.bind(portSerwera) != sf::Socket::Done)
		{
			std::cout << "Blad bindowania portu" << std::endl;
			std::cin.ignore(2);
			exit(-1);
		}
		system("cls");
		std::cout << "Serwer UDP  " << sf::IpAddress::getLocalAddress().toString() << ":" << socket.getLocalPort() << std::endl;
	}

	// funkcja wysy³a pakiet do danego klienta
	void sendpakiet(pakiet& t, std::pair<sf::IpAddress, unsigned short> h)
	{
		char* wyslij = t.toCharArray();
		if (socket.send(wyslij, t.getSize(), h.first, h.second))
		{
			std::cout << "Blad wysylania danych." << std::endl;
		}
		else
		{
			std::cout << ">>>> " << h.first.toString() << ":" << h.second << " <- " << t.toString() << std::endl;
		}
		delete[] wyslij;
	}

	// funkcja wysy³a ACK do danego klienta
	void sendACK(std::pair<sf::IpAddress, unsigned short> h, sf::Uint8 session)
	{
		char* dane;
		pakiet pakiet;
		pakiet.komunikaty.ACK = 1;
		pakiet.identyfikator = session;

		dane = pakiet.toCharArray();
		if (socket.send(dane, pakiet.getSize(), h.first, h.second) != sf::Socket::Done)
		{
			std::cout << "Blad wysylania ACK." << std::endl;
		}
		else
		{
			std::cout << ">>>> " << h.first.toString() << ":" << h.second << " <- " << pakiet.toString() << std::endl;
		}
		delete[] dane;
	}

	// funkcja odbiera pakiet od klienta
	HOST receivepakiet(pakiet& p)
	{
		char* odbierz = new char[rozmiarBufora];
		size_t odebrano;

		sf::IpAddress nadawca;
		unsigned short portNadawcy;
		if (socket.receive(odbierz, rozmiarBufora, odebrano, nadawca, portNadawcy) != sf::Socket::Done)
		{
			std::cout << "Blad odbierania danych" << std::endl;
		}
		else
		{
			// wype³nienie pakietu p odebranymi danymi
			p.fromCharArray(odbierz, odebrano);
			std::cout << "<<<< " << nadawca.toString() << ":" << portNadawcy << " -> " << p.toString() << std::endl;

			// jeœli pakiet nie by³ potwierdzeniem, to odsy³ane jest ACK 
			// potwierdzenie nie jest wysy³ane klientom bez ID
			if (p.identyfikator != 0 && p.komunikaty.ACK == 0)
			{
				sendACK({ nadawca,portNadawcy }, p.identyfikator);
			}
		}
		delete[] odbierz;

		// zwraca informacje o nadawcy pakietu
		HOST host{ nadawca, portNadawcy };
		return{ host };
	}

	void run()
	{
		while (true)
		{
			std::cout << "\nOczekiwanie na nastepny pakiet..." << std::endl;
			pakiet aktualny_pakiet;
			// odebranie pakietu
			auto aktualny_klient = receivepakiet(aktualny_pakiet);

			if (aktualny_pakiet.komunikaty.ACK)
			{
				std::cout << "        " << aktualny_klient.first.toString() << ":" << aktualny_klient.second << " ACK" << std::endl;
			}
			else if (aktualny_pakiet.komunikaty.ID)
			{
				pakiet odpowiedz;
				do {
					// wylosowanie ID dla klienta, który wys³a³ pakiet
					sf::Uint8 random;
					random = rand() % 254 + 1;
					if (random != host1.first && random != host2.first)
					{
						// zapamiêtanie danych klienta
						if (host1.first == 0)
						{
							host1.first = random; //ID klienta
							host1.second.first = aktualny_klient.first; //adres IP
							host1.second.second = aktualny_klient.second; //port

							odpowiedz.identyfikator = random;
							odpowiedz.komunikaty.ACK = 1;
							std::cout << "klient 1 zostal polaczony\n ID sesji:" << (unsigned int)host1.first << " " << host1.second.first << ":" << host1.second.second << std::endl;
						}
						else if (host2.first == 0)
						{
							host2.first = random; //ID klienta
							host2.second.first = aktualny_klient.first; //adres IP
							host2.second.second = aktualny_klient.second; //port

							odpowiedz.identyfikator = random;
							odpowiedz.komunikaty.ACK = 1;
							std::cout << "klient 2 zostal polaczony\n ID sesji:" << (unsigned int)host2.first << ", " << host1.second.first << ":" << host1.second.second << std::endl;
						}
						else
						{
							std::cout << "proba poloczenia zbyt duzej liczby klientow" << std::endl;
							std::cin.ignore(2);
							exit(-1);
						}
					}

				} while (odpowiedz.identyfikator == 0);

				sendpakiet(odpowiedz, aktualny_klient);
			}
			else if (aktualny_pakiet.komunikaty.KON)
			{
				// roz³¹czanie klientów
				if (host1.first == aktualny_pakiet.identyfikator)
				{
					std::cout << "klient 1 zostal rozloczony" << std::endl;
					host1.first = 0;
					komunikacja_klientow = 0;
				}
				else if (host2.first == aktualny_pakiet.identyfikator)
				{
					std::cout << "klient 2 zostal rozloczony" << std::endl;
					host2.first = 0;
					komunikacja_klientow = 0;
				}
				// gdy nie ma ju¿ ¿adnych klientów - wy³¹czenie serwera
				if (host1.first == 0 && host2.first == 0) { sf::sleep(sf::seconds(3)); return; }
			}
			else if (aktualny_pakiet.komunikaty.ZAPR)
			{
				// jeœli klienci nie s¹ po³¹czeni
				if (!komunikacja_klientow)
				{
					if (host1.first && host2.first)
					{
						if (aktualny_pakiet.identyfikator == host1.first)
						{
							aktualny_pakiet.identyfikator = host2.first; // zmiana ID sesji na ID drugiego klienta
							sendpakiet(aktualny_pakiet, host2.second); // przesy³anie do drugiego klienta
						}
						else if (aktualny_pakiet.identyfikator == host2.first)
						{
							aktualny_pakiet.identyfikator = host1.first; // zmiana ID sesji na ID drugiego klienta
							sendpakiet(aktualny_pakiet, host1.second); // przesy³anie do drugiego klienta
						}
					}
					else
					{
						pakiet kom;
						kom.identyfikator = aktualny_pakiet.identyfikator;
						kom.komunikaty.Z_TAK = 1;
						kom.komunikaty.Z_NIE = 1;
						sendpakiet(kom, aktualny_klient);
					}
				}
			}
			else if (aktualny_pakiet.komunikaty.Z_TAK)
			{
				// przekazanie odebranego pakietu do drugiego klienta
				komunikacja_klientow = 1;
				if (aktualny_klient == host1.second)
				{
					aktualny_pakiet.identyfikator = host2.first;
					sendpakiet(aktualny_pakiet, host2.second);
				}
				else if (aktualny_klient == host2.second)
				{
					aktualny_pakiet.identyfikator = host1.first;
					sendpakiet(aktualny_pakiet, host1.second);
				}
			}
			else if (aktualny_pakiet.komunikaty.Z_NIE)
			{
				// przekazanie odebranego pakietu do drugiego klienta - analogicznie jak dla ZAPR
				if (aktualny_klient == host1.second)
				{
					aktualny_pakiet.identyfikator = host2.first;
					sendpakiet(aktualny_pakiet, host2.second);
				}
				else if (aktualny_klient == host2.second)
				{
					aktualny_pakiet.identyfikator = host1.first;
					sendpakiet(aktualny_pakiet, host1.second);
				}
			}
			else
			{
				// odebranie pakietu, w którym nie jest ustawiona ¿adna flaga (ZAPR, Z_TAK itd.), oznacza, ¿e przesy³ana jest wiadomoœæ
				if (komunikacja_klientow)
				{
					// je¿eli klienci s¹ po³¹czeni
					// przekazanie odebranego pakietu do drugiego klienta - analogicznie jak dla ZAPR
					if (aktualny_klient == host1.second)
					{
						aktualny_pakiet.identyfikator = host2.first;
						sendpakiet(aktualny_pakiet, host2.second);
					}
					else if (aktualny_klient == host2.second)
					{
						aktualny_pakiet.identyfikator = host1.first;
						sendpakiet(aktualny_pakiet, host1.second);
					}
				}
				else
				{
					// Jeœli klienci siê nie po³aczyli to serwer odsy³a wiadomœæ do klienta
					pakiet kom;
					if (aktualny_klient.first == host1.second.first)
						kom.identyfikator = host1.first;
					else
						kom.identyfikator = host2.first;

					kom.komunikaty.Z_TAK = 1;
					kom.komunikaty.Z_NIE = 1;
					sendpakiet(kom, aktualny_klient);
				}
			}
		}
		sf::sleep(sf::seconds(10));
	}
};