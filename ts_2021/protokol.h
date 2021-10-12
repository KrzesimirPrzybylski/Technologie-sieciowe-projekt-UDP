#pragma once
#ifndef protokol_h
#define protokol_h
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <sstream>
#include <time.h>
#include <bitset>
#include <algorithm>
#include <locale.h>
#include <boost/dynamic_bitset.hpp>
#include <thread>

const unsigned int rozmiarBufora = 4096;

// Funkcja odwraca kolejnoœæ bitów
template <std::size_t s>
void bitReverse(std::bitset<s>& b)
{
	auto temp = b;
	for (int i = 0; i < s; i++)
	{
		b[i] = temp[s - 1 - i];
	}
}

// funkcja zlicza jedynki w protokole
template <std::size_t x>
int S_KON(std::bitset<x>& b) {
	int k=0;
	for (int i = 0; i < x; i++)
	{
		if (b[i] == 1)
			k++;
	}
	return k;
}

//Klasa przechowuje komunikaty przesy³ane przez klientów i serwer
class flags
{
public:
	/*
	u¿yte komunikaty ("flagi")
	ID - pobierz id sesji
	ZAPR - zaproœ drugiego klienta
	KON - zakoñcz po³¹czenie
	ACK - ACK
	Z_TAK - przyjêcie zaproszenia
	Z_NIE - odrzucenie zaproszenia
	*/
	void reset() { ID = 0; ZAPR = 0; KON = 0; ACK = 0; Z_TAK = 0; Z_NIE = 0; P = 0; }
	flags() { ID = 0; ZAPR = 0; KON = 0; ACK = 0; Z_TAK = 0; Z_NIE = 0; P = 0; }
	bool
		P : 1,
		ID : 1,
		ZAPR : 1,
		KON : 1,
		: 1,
		ACK : 1,
		Z_TAK : 1,
		Z_NIE : 1,
		: 1;
	//wypisanie pakietu 
	std::string toString()
	{
		std::string result = "[Pole operacji: ";
		if (ID) result += " ID";
		if (ZAPR) result += " ZAPR";
		if (KON) result += " KON";
		result += "] "; 
		result += "[Pole odpowiedzi: ";
		if (ACK) result += " ACK";
		if (Z_TAK) result += " Z_TAK";
		if (Z_NIE) result += " Z_NIE";
		return result += "] ";
	}
};

// klasa przechowuj¹ca pakiety protoko³u
class pakiet
{
public:
	pakiet() :identyfikator(0), komunikaty(), dlugoscDanych(0), dane(nullptr), sum_kon(0){}
	~pakiet() { }

	flags komunikaty;
	sf::Uint32 dlugoscDanych;
	char* dane;
	sf::Uint8 identyfikator;
	sf::Uint8 sum_kon;

	void clear()
	{
		identyfikator = 0;
		komunikaty.reset();
		dlugoscDanych = 0;

		if (dane) delete[] dane;
		dane = nullptr;
	}

	// funkcja zamienia tablicê znaków na pakiet
	void fromCharArray(const char* arr, std::size_t size)
	{
		boost::dynamic_bitset<> dbitset = char_to_bitSet(arr, size);
		from_bitSet(dbitset);
	}

	// funkcja zamienia pakiet na tablicê znaków
	char* toCharArray()
	{
		char* result = bitSet_to_char(this->to_bitSet());
		return result;
	}

	unsigned int getSize()
	{
		return sizeof(identyfikator) + sizeof(komunikaty) + sizeof(dlugoscDanych) + dlugoscDanych + 1;
	}

	// wypisanie pakietu
	std::string toString()
	{
		std::ostringstream result;
		result << "[ID: " << unsigned int(identyfikator) << "] " << komunikaty.toString()  << "[Dlugosc: " << dlugoscDanych << "] ";
		for (int i = 0; i < dlugoscDanych; i++)
		{
			result << dane[i];
		}
		return result.str();
	}

	// pobieranie tekstu z pakietu
	std::string getText()
	{
		std::string result;
		for (int i = 0; i < dlugoscDanych; i++)
		{
			result += dane[i];
		}
		return result;
	}



private:

	
	//funkcja ustawia bity w dobrej kolejnoœci
	boost::dynamic_bitset<> to_bitSet()
	{
		std::bitset<8> sum_k = 0;
		boost::dynamic_bitset<> result;
		boost::dynamic_bitset<> result2;
		for (int i = 7; i > 0; i--)
		{
			result.push_back(0);
		}

		std::bitset<8> temp2(identyfikator);
		for (int i = 0; i < 8; i++)
		{
			result.push_back(temp2[i]);
		}

		for (int i = dlugoscDanych - 1; i >= 0; i--)
		{
			std::bitset<8> znak(dane[i]);
			for (int i = 0; i < 8; i++)
			{
				result.push_back(znak[i]);
			}
		}

		std::bitset<32> temp(dlugoscDanych);
		for (int i = 0; i < 32; i++)
		{
			result.push_back(temp[i]);
		}
		result.push_back(komunikaty.Z_NIE);
		result.push_back(komunikaty.Z_TAK);
		result.push_back(komunikaty.ACK);
		result.push_back(komunikaty.P);
		result.push_back(komunikaty.P);
		result.push_back(komunikaty.KON);
		result.push_back(komunikaty.ZAPR);
		result.push_back(komunikaty.ID);
		result.push_back(komunikaty.P);
		sum_k = suma_kontrolna(result);

		for (int i = 0; i <8; i++)
		{
			result2.push_back(sum_k[i]);
		}
		for (int i = 0; i < result.size(); i++)
			result2.push_back(result[i]);
		return result2;
	}

	

	std::bitset<8> suma_kontrolna(boost::dynamic_bitset<>& bitset)
	{
		int l = 0;
		for (int i = 0; i < bitset.size(); i++) {
			if (bitset[i] == 1)
				l++;
		}
		std::bitset<8> licznik(l);
		return licznik;
	}

	// funkcja wypa³nia pakiet odebranymi danymi
	void from_bitSet(boost::dynamic_bitset<>& bitset)
	{
		this->komunikaty.P = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.ID = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.ZAPR = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.KON = bitset[bitset.size() - 1];
		bitset.pop_back();

		this->komunikaty.P = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.P = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.ACK = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.Z_TAK = bitset[bitset.size() - 1];
		bitset.pop_back();
		this->komunikaty.Z_NIE = bitset[bitset.size() - 1];
		bitset.pop_back();

		std::bitset<32> temp;
		for (int i = 31; i >= 0; i--)
		{
			temp[i] = bitset[bitset.size() - 1]; bitset.pop_back();
		}
		this->dlugoscDanych = temp.to_ulong();

		if (dane) delete[] dane;
		dane = new char[dlugoscDanych];

		for (int i = 0; i < dlugoscDanych; i++)
		{
			std::bitset<8> znak;
			for (int i = 7; i >= 0; i--)
			{
				znak[i] = bitset[bitset.size() - 1];  bitset.pop_back();
			}
			dane[i] = znak.to_ulong();
		}

		std::bitset<8> temp2;
		for (int i = 7; i >= 0; i--)
		{
			temp2[i] = bitset[bitset.size() - 1];  bitset.pop_back();
		}
		identyfikator = temp2.to_ulong();

		for (int i = 6; i >= 0; i--)
		{
			bitset.pop_back();
		}
	}

	// funkcja zamienia tablicê znaków na bitset
	static boost::dynamic_bitset<> char_to_bitSet(const char* arr, std::size_t size)
	{
		boost::dynamic_bitset<> result;
		for (int i = size - 1; i >= 0; i--)
		{
			for (int j = 0; j < 8; j++)
			{
				result.push_back((arr[i] >> j) & 1);
			}
		}
		return result;
	}
	// funkcja zamienia bitset na tablicê znaków
	static char* bitSet_to_char(boost::dynamic_bitset<> b)
	{
		char* result = new char[b.size() / 8];

		for (int i = 0; i < b.size() / 8; i++)
		{
			std::bitset<8> znak;

			for (int j = 0; j < 8; j++)
			{
				znak[j] = b[(b.size() - j - 1 - i * 8)];
			}
			bitReverse(znak);
			result[i] = znak.to_ulong();
		}
		return result;
	}
};


// operator do dodania do pakietu tekstu
pakiet& operator<<(pakiet& p, const std::string& message)
{
	p.dlugoscDanych = message.size();
	if (p.dlugoscDanych > rozmiarBufora - (sizeof(p.identyfikator) + sizeof(p.komunikaty) + sizeof(p.dlugoscDanych) + sizeof(p.sum_kon)+8))
	{
		p.dlugoscDanych = rozmiarBufora - (sizeof(p.identyfikator) + sizeof(p.komunikaty) + sizeof(p.dlugoscDanych) + sizeof(p.sum_kon)+8);
	}
	if (p.dane) delete[] p.dane;

	p.dane = new char[p.dlugoscDanych];

	for (int i = 0; i < p.dlugoscDanych; i++)
	{
		p.dane[i] = message[i];
	}
	return p;
}
#endif