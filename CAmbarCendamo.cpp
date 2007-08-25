#include "stdafx.h"
#include "CAmbarCendamo.h"
#include "CSemiDefHandler.h"
#include "CGameInfo.h"
#include "CObjectHandler.h"
#include "CCastleHandler.h"
#include "SDL_Extensions.h"
#include "boost\filesystem.hpp"
#include <set>
#include <iomanip>
#include <sstream>

unsigned int intPow(unsigned int a, unsigned int b)
{
	unsigned int ret=1;
	for(int i=0; i<b; ++i)
		ret*=a;
	return ret;
}
CAmbarCendamo::CAmbarCendamo (unsigned char * map)
{
	bufor=map;
}
CAmbarCendamo::CAmbarCendamo (const char * tie)
{
	is = new std::ifstream();
	is -> open(tie,std::ios::binary);
	is->seekg(0,std::ios::end); // na koniec
	andame = is->tellg();  // read length
	is->seekg(0,std::ios::beg); // wracamy na poczatek
	bufor = new unsigned char[andame]; // allocate memory 
	is->read((char*)bufor, andame); // read map file to buffer
	is->close();
	delete is;
}
CAmbarCendamo::~CAmbarCendamo () 
{// free memory
	for (int ii=0;ii<map.width;ii++)
		delete map.terrain[ii] ; 
	delete map.terrain;
	delete bufor;
}
void CAmbarCendamo::teceDef()
{
	for (int i=0; i<map.defy.size(); i++)
	{
		std::ofstream * of = new std::ofstream(map.defy[i].name.c_str());
		for (int j=0;j<46;j++)
		{
			(*of) << map.defy[i].bytes[j]<<std::endl;
		}
		of->close();
		delete of;
	}
}
void CAmbarCendamo::deh3m()
{
	THC timeHandler th;
	map.version = (Eformat)bufor[0]; //wersja mapy
	map.areAnyPLayers = bufor[4]; //invalid on some maps
	map.height = map.width = bufor[5]; // wymiary mapy
	map.twoLevel = bufor[9]; //czy sa lochy
	map.terrain = new TerrainTile*[map.width]; // allocate memory 
	for (int ii=0;ii<map.width;ii++)
		map.terrain[ii] = new TerrainTile[map.height]; // allocate memory 
	if (map.twoLevel)
	{
		map.undergroungTerrain = new TerrainTile*[map.width]; // allocate memory 
		for (int ii=0;ii<map.width;ii++)
			map.undergroungTerrain[ii] = new TerrainTile[map.height]; // allocate memory 
	}
	int length = bufor[10]; //name length
	int i=14, pom; 
	while (i-14<length)	//read name
		map.name+=bufor[i++];
	length = bufor[i] + bufor[i+1]*256; //description length
	i+=4;
	for (pom=0;pom<length;pom++)
		map.description+=bufor[i++];
	map.difficulty = bufor[i++]; // reading map difficulty
	if(map.version != Eformat::RoE)
	{
		map.levelLimit = bufor[i++]; // hero level limit
	}
	else
	{
		map.levelLimit = 0;
	}
	for (pom=0;pom<8;pom++)
	{
		map.players[pom].canHumanPlay = bufor[i++];
		map.players[pom].canComputerPlay = bufor[i++];
		if ((!(map.players[pom].canHumanPlay || map.players[pom].canComputerPlay)))
		{
			switch(map.version)
			{
			case Eformat::SoD: case Eformat::WoG: 
				i+=13;
				break;
			case Eformat::AB:
				i+=12;
				break;
			case Eformat::RoE:
				i+=6;
				break;
			}
			continue;
		}

		map.players[pom].AITactic = bufor[i++];

		if(map.version == Eformat::SoD || map.version == Eformat::WoG)
			i++;

		map.players[pom].allowedFactions = 0;
		map.players[pom].allowedFactions += bufor[i++];
		if(map.version != Eformat::RoE)
			map.players[pom].allowedFactions += (bufor[i++])*256;

		map.players[pom].isFactionRandom = bufor[i++];
		map.players[pom].hasMainTown = bufor[i++];
		if (map.players[pom].hasMainTown)
		{
			if(map.version != Eformat::RoE)
			{
				map.players[pom].generateHeroAtMainTown = bufor[i++];
				map.players[pom].generateHero = bufor[i++];
			}
			map.players[pom].posOfMainTown.x = bufor[i++];
			map.players[pom].posOfMainTown.y = bufor[i++];
			map.players[pom].posOfMainTown.z = bufor[i++];

			
		}

		i++; //unknown
		
		if(bufor[i++]!=0xff)
		{
			map.players[pom].mainHeroPortrait = bufor[i++];
			int nameLength = bufor[i++];
			i+=3; 
			for (int pp=0;pp<nameLength;pp++)
				map.players[pom].mainHeroName+=bufor[i++];
		}

		//i++; //unknown byte
		if(map.version != Eformat::RoE)
		{
			i++; ////unknown byte
			int heroCount = bufor[i++];
			i+=3;
			for (int pp=0;pp<heroCount;pp++)
			{
				SheroName vv;
				vv.heroID=bufor[i++];
				int hnl = bufor[i++];
				i+=3;
				for (int zz=0;zz<hnl;zz++)
				{
					vv.heroName+=bufor[i++];
				}
				map.players[pom].heroesNames.push_back(vv);
			}
		}
	}
	map.victoryCondition = (EvictoryConditions)bufor[i++];
	if (map.victoryCondition != winStandard) //specific victory conditions
	{
		int nr;
		switch (map.victoryCondition) //read victory conditions
		{
		case artifact:
			{
				map.vicConDetails = new VicCon0();
				((VicCon0*)map.vicConDetails)->ArtifactID = bufor[i+2];
				nr=(map.version==RoE ? 1 : 2);
				break;
			}
		case gatherTroop:
			{
				map.vicConDetails = new VicCon1();
				int temp1 = bufor[i+2];
				int temp2 = bufor[i+3];
				((VicCon1*)map.vicConDetails)->monsterID = bufor[i+2];
				((VicCon1*)map.vicConDetails)->neededQuantity=readNormalNr(i+(map.version==RoE ? 3 : 4));
				nr=(map.version==RoE ? 5 : 6);
				break;
			}
		case gatherResource:
			{
				map.vicConDetails = new VicCon2();
				((VicCon2*)map.vicConDetails)->resourceID = bufor[i+2];
				((VicCon2*)map.vicConDetails)->neededQuantity=readNormalNr(i+3);
				nr=5;
				break;
			}
		case buildCity:
			{
				map.vicConDetails = new VicCon3();
				((VicCon3*)map.vicConDetails)->posOfCity.x = bufor[i+2];
				((VicCon3*)map.vicConDetails)->posOfCity.y = bufor[i+3];
				((VicCon3*)map.vicConDetails)->posOfCity.z = bufor[i+4];
				((VicCon3*)map.vicConDetails)->councilNeededLevel = bufor[i+5];
				((VicCon3*)map.vicConDetails)->fortNeededLevel = bufor[i+6];
				nr=5;
				break;
			}
		case buildGrail:
			{
				map.vicConDetails = new VicCon4();
				if (bufor[i+4]>2)
					((VicCon4*)map.vicConDetails)->anyLocation = true;
				else
				{
					((VicCon4*)map.vicConDetails)->whereBuildGrail.x = bufor[i+2];
					((VicCon4*)map.vicConDetails)->whereBuildGrail.y = bufor[i+3];
					((VicCon4*)map.vicConDetails)->whereBuildGrail.z = bufor[i+4];
				}
				nr=3;
				break;
			}
		case beatHero:
			{
				map.vicConDetails = new VicCon5();
				((VicCon5*)map.vicConDetails)->locationOfHero.x = bufor[i+2];
				((VicCon5*)map.vicConDetails)->locationOfHero.y = bufor[i+3];
				((VicCon5*)map.vicConDetails)->locationOfHero.z = bufor[i+4];				
				nr=3;
				break;
			}
		case captureCity:
			{
				map.vicConDetails = new VicCon6();
				((VicCon6*)map.vicConDetails)->locationOfTown.x = bufor[i+2];
				((VicCon6*)map.vicConDetails)->locationOfTown.y = bufor[i+3];
				((VicCon6*)map.vicConDetails)->locationOfTown.z = bufor[i+4];				
				nr=3;
				break;
			}
		case beatMonster:
			{
				map.vicConDetails = new VicCon7();
				((VicCon7*)map.vicConDetails)->locationOfMonster.x = bufor[i+2];
				((VicCon7*)map.vicConDetails)->locationOfMonster.y = bufor[i+3];
				((VicCon7*)map.vicConDetails)->locationOfMonster.z = bufor[i+4];				
				nr=3;
				break;
			}
		case takeDwellings:
			{		
				map.vicConDetails = new CspecificVictoryConidtions();
				nr=0;
				break;
			}
		case takeMines:
			{	
				map.vicConDetails = new CspecificVictoryConidtions();	
				nr=0;
				break;
			}
		case transportItem:
			{
				map.vicConDetails = new VicCona();
				((VicCona*)map.vicConDetails)->artifactID =  bufor[i+2];
				((VicCona*)map.vicConDetails)->destinationPlace.x = bufor[i+3];
				((VicCona*)map.vicConDetails)->destinationPlace.y = bufor[i+4];
				((VicCona*)map.vicConDetails)->destinationPlace.z = bufor[i+5];				
				nr=3;
				break;
			}
		}
		map.vicConDetails->allowNormalVictory = bufor[i++];
		map.vicConDetails->appliesToAI = bufor[i++];
		i+=nr;
	}
	map.lossCondition.typeOfLossCon = (ElossCon)bufor[i++];
	switch (map.lossCondition.typeOfLossCon) //read loss conditions
	{
	case lossCastle:
		  {
			  map.lossCondition.castlePos.x=bufor[i++];
			  map.lossCondition.castlePos.y=bufor[i++];
			  map.lossCondition.castlePos.z=bufor[i++];
			  break;
		  }
	case lossHero:
		  {
			  map.lossCondition.heroPos.x=bufor[i++];
			  map.lossCondition.heroPos.y=bufor[i++];
			  map.lossCondition.heroPos.z=bufor[i++];
			  break;
		  }
	case timeExpires:
		{
			map.lossCondition.timeLimit = readNormalNr(i++,2);
			i++;
			 break;
		}
	}
	map.howManyTeams=bufor[i++]; //read number of teams
	if(map.howManyTeams>0) //read team numbers
	{
		for(int rr=0; rr<8; ++rr)
		{
			map.players[rr].team=bufor[i++];
		}
	}
	//reading allowed heroes (20 bytes)
	int ist;

	ist=i; //starting i for loop
	for(i; i<ist+ (map.version == Eformat::RoE ? 16 : 20) ; ++i)
	{
		unsigned char c = bufor[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < CGameInfo::mainObj->heroh->heroes.size())
			{
				if(c == (c|((unsigned char)intPow(2, yy))))
					CGameInfo::mainObj->heroh->heroes[(i-ist)*8+yy]->isAllowed = true;
				else
					CGameInfo::mainObj->heroh->heroes[(i-ist)*8+yy]->isAllowed = false;
			}
		}
	}
	if(map.version>=RoE)
		i+=4;
	unsigned char disp = 0;
	if(map.version>=SoD)
	{
		disp = bufor[i++];
		for(int g=0; g<disp; ++g)
		{
			i+=2;
			int lenbuf = readNormalNr(i); i+=4;
			i+=lenbuf+1;
		}
	}
	//allowed heroes have been read
	unsigned char aaa1=bufor[i], aaa2=bufor[i+1], aaa3=bufor[i+2];
	i+=31; //omitting 0s
	//reading allowed artifacts //18 bytes
	if(map.version!=RoE)
	{
		ist=i; //starting i for loop
		for(i; i<ist+18; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < CGameInfo::mainObj->arth->artifacts.size())
				{
					if(c != (c|((unsigned char)intPow(2, yy))))
						CGameInfo::mainObj->arth->artifacts[(i-ist)*8+yy].isAllowed = true;
					else
						CGameInfo::mainObj->arth->artifacts[(i-ist)*8+yy].isAllowed = false;
				}
			}
		}//allowed artifacts have been read
	}

	//reading allowed spells (9 bytes)
	if(map.version>=SoD)
	{
		ist=i; //starting i for loop
		for(i; i<ist+9; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
				{
					if(c != (c|((unsigned char)intPow(2, yy))))
						CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy].isAllowed = true;
					else
						CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy].isAllowed = false;
				}
			}
		}
		//allowed spells have been read
		//allowed hero's abilities (4 bytes)
		ist=i; //starting i for loop
		for(i; i<ist+4; ++i)
		{
			unsigned char c = bufor[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < CGameInfo::mainObj->abilh->abilities.size())
				{
					if(c != (c|((unsigned char)intPow(2, yy))))
						CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]->isAllowed = true;
					else
						CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]->isAllowed = false;
				}
			}
		}
	}
	//allowed hero's abilities have been read

	THC std::cout<<"Reading header: "<<th.getDif()<<std::endl;
	int rumNr = readNormalNr(i,4);i+=4;
	for (int it=0;it<rumNr;it++)
	{
		Rumor ourRumor;
		int nameL = readNormalNr(i,4);i+=4; //read length of name of rumor
		for (int zz=0; zz<nameL; zz++)
			ourRumor.name+=bufor[i++];
		nameL = readNormalNr(i,4);i+=4; //read length of rumor
		for (int zz=0; zz<nameL; zz++)
			ourRumor.text+=bufor[i++];
		map.rumors.push_back(ourRumor); //add to our list
	}
	THC std::cout<<"Reading rumors: "<<th.getDif()<<std::endl;
	switch(map.version)
	{
	case WoG: case SoD: case AB:
		if(bufor[i]=='\0') //omit 156 bytes of rubbish
		{
			i+=156;
			break;
		}
		else //omit a lot of rubbish in a strage way
		{
			int lastFFpos=i;
			while(i-lastFFpos<50) //i far in terrain bytes
			{
				++i;
				if(bufor[i]==0xff)
				{
					lastFFpos=i;
				}
			}

			i=lastFFpos;

			while(bufor[i-1]!=0 || bufor[i]>8 || bufor[i+2]>4 || bufor[i+1]==0) //back to terrain bytes
			{
				i++;
			}
		}
		break;
	case RoE:
		i+=0;
		break;
	}
	for (int c=0; c<map.width; c++) // reading terrain
	{
		for (int z=0; z<map.height; z++)
		{
			map.terrain[z][c].tertype = (EterrainType)(bufor[i++]);
			map.terrain[z][c].terview = bufor[i++];
			map.terrain[z][c].nuine = (Eriver)bufor[i++];
			map.terrain[z][c].rivDir = bufor[i++];
			map.terrain[z][c].malle = (Eroad)bufor[i++];
			map.terrain[z][c].roadDir = bufor[i++];
			map.terrain[z][c].siodmyTajemniczyBajt = bufor[i++];
		}
	}
	if (map.twoLevel) // read underground terrain
	{
		for (int c=0; c<map.width; c++) // reading terrain
		{
			for (int z=0; z<map.height; z++)
			{
				map.undergroungTerrain[z][c].tertype = (EterrainType)(bufor[i++]);
				map.undergroungTerrain[z][c].terview = bufor[i++];
				map.undergroungTerrain[z][c].nuine = (Eriver)bufor[i++];
				map.undergroungTerrain[z][c].rivDir = bufor[i++];
				map.undergroungTerrain[z][c].malle = (Eroad)bufor[i++];
				map.undergroungTerrain[z][c].roadDir = bufor[i++];
				map.undergroungTerrain[z][c].siodmyTajemniczyBajt = bufor[i++];
			}
		}
	}
	THC std::cout<<"Reading terrain: "<<th.getDif()<<std::endl;
	int defAmount = bufor[i]; // liczba defow
	defAmount = readNormalNr(i);
	i+=4;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int srmask = 0xff000000;
    int sgmask = 0x00ff0000;
    int sbmask = 0x0000ff00;
    int samask = 0x000000ff;
#else
    int srmask = 0x000000ff;
    int sgmask = 0x0000ff00;
    int sbmask = 0x00ff0000;
    int samask = 0xff000000;
#endif
	SDL_Surface * alphaTransSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, 12, 12, 32, srmask, sgmask, sbmask, samask);
	std::vector<std::string> defsToUnpack;
	for (int idd = 0 ; idd<defAmount; idd++) // reading defs
	{
		int nameLength = readNormalNr(i,4);i+=4;
		DefInfo vinya; // info about new def
		for (int cd=0;cd<nameLength;cd++)
		{
			vinya.name += bufor[i++];
		}
		for (int v=0; v<42; v++) // read info
		{
			vinya.bytes[v] = bufor[i++];
		}
		std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
			vinya.name);
		if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
		{
			vinya.isOnDefList = false;
		}
		else
		{
			vinya.printPriority = pit->priority;
			vinya.isOnDefList = true;
		}
		map.defy.push_back(vinya); // add this def to the vector
		defsToUnpack.push_back(vinya.name);
	}
	THC std::cout<<"Reading defs: "<<th.getDif()<<std::endl;
	////loading objects
	int howManyObjs = readNormalNr(i, 4); i+=4;
	for(int ww=0; ww<howManyObjs; ++ww) //comment this line to turn loading objects off
	{
		//std::cout << "object nr "<<ww<<"\ti= "<<i<<std::endl;
		CObjectInstance * nobj = new CObjectInstance; //we will read this object
		nobj->id = CGameInfo::mainObj->objh->objInstances.size();
		nobj->pos.x = bufor[i++];
		nobj->pos.y = bufor[i++];
		nobj->pos.z = bufor[i++];
		nobj->defNumber = readNormalNr(i, 4); i+=4;
		nobj->defObjInfoNumber = -1;

		//if (((nobj.x==0)&&(nobj.y==0)) || nobj.x>map.width || nobj.y>map.height || nobj.z>1 || nobj.defNumber>map.defy.size())
		//	std::cout << "Alarm!!! Obiekt "<<ww<<" jest kopniety (lub wystaje poza mape)\n";

		i+=5;
		unsigned char buff [30];
		for(int ccc=0; ccc<30; ++ccc)
		{
			buff[ccc] = bufor[i+ccc];
		}
		EDefType uu = getDefType(map.defy[nobj->defNumber]);
		int j = map.defy[nobj->defNumber].bytes[16];
		int p = 99;
		switch(getDefType(map.defy[nobj->defNumber]))
		{
		case EDefType::EVENTOBJ_DEF: //for event - objects
			{
				CEventObjInfo * spec = new CEventObjInfo;
				bool guardMess;
				guardMess = bufor[i]; ++i;
				if(guardMess)
				{
					int messLong = readNormalNr(i, 4); i+=4;
					if(messLong>0)
					{
						spec->isMessage = true;
						for(int yy=0; yy<messLong; ++yy)
						{
							spec->message +=bufor[i+yy];
						}
						i+=messLong;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(i); i+=28;
					}
					i+=4;
				}
				else
				{
					spec->isMessage = false;
					spec->areGuarders = false;
					spec->message = std::string("");
				}
				spec->gainedExp = readNormalNr(i, 4); i+=4;
				spec->manaDiff = readNormalNr(i, 4); i+=4;
				spec->moraleDiff = readNormalNr(i, 1, true); ++i;
				spec->luckDiff = readNormalNr(i, 1, true); ++i;
				spec->wood = readNormalNr(i); i+=4;
				spec->mercury = readNormalNr(i); i+=4;
				spec->ore = readNormalNr(i); i+=4;
				spec->sulfur = readNormalNr(i); i+=4;
				spec->crystal = readNormalNr(i); i+=4;
				spec->gems = readNormalNr(i); i+=4;
				spec->gold = readNormalNr(i); i+=4;
				spec->attack = readNormalNr(i, 1); ++i;
				spec->defence = readNormalNr(i, 1); ++i;
				spec->power = readNormalNr(i, 1); ++i;
				spec->knowledge = readNormalNr(i, 1); ++i;
				int gabn; //number of gained abilities
				gabn = readNormalNr(i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					spec->abilities.push_back((CGameInfo::mainObj->abilh)->abilities[readNormalNr(i, 1)]); ++i;
					spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
				}
				int gart = readNormalNr(i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					spec->artifacts.push_back(&(CGameInfo::mainObj->arth->artifacts[readNormalNr(i, 2)])); i+=2;
				}
				int gspel = readNormalNr(i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[readNormalNr(i, 1)])); ++i;
				}
				int gcre = readNormalNr(i, 1); ++i; //number of gained creatures
				spec->creatures = readCreatureSet(i, gcre); i+=4*gcre;
				i+=8;
				spec->availableFor = readNormalNr(i, 1); ++i;
				spec->computerActivate = readNormalNr(i, 1); ++i;
				spec->humanActivate = readNormalNr(i, 1); ++i;
				i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::HERO_DEF:
			{
				CHeroObjInfo * spec = new CHeroObjInfo;
				spec->bytes[0] = bufor[i]; ++i;
				spec->bytes[1] = bufor[i]; ++i;
				spec->bytes[2] = bufor[i]; ++i;
				spec->bytes[3] = bufor[i]; ++i;
				spec->player = bufor[i]; ++i;
				int typeBuf = readNormalNr(i, 1); ++i;
				if(typeBuf==0xff)
					spec->type = NULL;
				else
					spec->type = CGameInfo::mainObj->heroh->heroes[typeBuf];
				bool isName = bufor[i]; ++i; //true if hero has nonstandard name
				if(isName)
				{
					int length = readNormalNr(i, 4); i+=4;
					for(int gg=0; gg<length; ++gg)
					{
						spec->name+=bufor[i]; ++i;
					}
				}
				else
					spec->name = std::string("");
				bool isExp = bufor[i]; ++i; //true if hore's experience is greater than 0
				if(isExp)
				{
					spec->experience = readNormalNr(i); i+=4;
				}
				else spec->experience = 0;
				bool portrait=bufor[i]; ++i;
				if (portrait)
					i++; //TODO read portrait nr, save, open
				bool nonstandardAbilities = bufor[i]; ++i; //true if hero has specified abilities
				if(nonstandardAbilities)
				{
					int howMany = readNormalNr(i); i+=4;
					for(int yy=0; yy<howMany; ++yy)
					{
						spec->abilities.push_back(CGameInfo::mainObj->abilh->abilities[readNormalNr(i, 1)]); ++i;
						spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
					}
				}
				bool standGarrison = bufor[i]; ++i; //true if hero has nonstandard garrison
				spec->standardGarrison = standGarrison;
				if(standGarrison)
				{
					spec->garrison = readCreatureSet(i); i+=28; //4 bytes per slot
				}
				bool form = bufor[i]; ++i; //formation
				spec->garrison.formation = form;
				bool artSet = bufor[i]; ++i; //true if artifact set is not default (hero has some artifacts)
				if(artSet)
				{
					//head art //1
					int id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artHead = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artHead = NULL;
					//shoulders art //2
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artShoulders = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artShoulders = NULL;
					//neck art //3
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artNeck = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artNeck = NULL;
					//right hand art //4
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artRhand = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artRhand = NULL;
					//left hand art //5
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artLHand = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artLHand = NULL;
					//torso art //6
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artTorso = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artTorso = NULL;
					//right hand ring //7
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artRRing = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artRRing = NULL;
					//left hand ring //8
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artLRing = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artLRing = NULL;
					//feet art //9
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artFeet = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artFeet = NULL;
					//misc1 art //10
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc1 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc1 = NULL;
					//misc2 art //11
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc2 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc2 = NULL;
					//misc3 art //12
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc3 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc3 = NULL;
					//misc4 art //13
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc4 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc4 = NULL;
					//machine1 art //14
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMach1 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach1 = NULL;
					//machine2 art //15
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMach2 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach2 = NULL;
					//machine3 art //16
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMach3 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMach3 = NULL;
					//misc5 art //17
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artMisc5 = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artMisc5 = NULL;
					//spellbook
					id = readNormalNr(i, 2); i+=2;
					if(id!=0xffff)
						spec->artSpellBook = &(CGameInfo::mainObj->arth->artifacts[id]);
					else
						spec->artSpellBook = NULL;
					//19 //???what is that? gap in file or what?
					i+=2;
					//bag artifacts //20
					int amount = readNormalNr(i, 2); i+=2; //number of artifacts in hero's bag
					if(amount>0)
					{
						for(int ss=0; ss<amount; ++ss)
						{
							id = readNormalNr(i, 2); i+=2;
							if(id!=0xffff)
								spec->artifacts.push_back(&(CGameInfo::mainObj->arth->artifacts[id]));
							else
								spec->artifacts.push_back(NULL);
						}
					}
				} //artifacts
				spec->guardRange = readNormalNr(i, 1); ++i;
				if(spec->guardRange == 0xff)
					spec->isGuarding = false;
				else
					spec->isGuarding = true;
				bool hasBiography = bufor[i]; ++i; //true if hero has nonstandard (mapmaker defined) biography
				if(hasBiography)
				{
					int length = readNormalNr(i); i+=4;
					int iStart = i;
					i+=length;
					for(int bb=0; bb<length; ++bb)
					{
						spec->biography+=bufor[iStart+bb];
					}
				}
				spec->sex = !(bufor[i]); ++i;
				//spells
				bool areSpells = bufor[i]; ++i;

				if(areSpells) //TODO: sprawdzi� //seems to be ok - tow
				{
					int ist = i;
					for(i; i<ist+9; ++i)
					{
						unsigned char c = bufor[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
							{
								if(c == (c|((unsigned char)intPow(2, yy))))
									spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy]));
							}
						}
					}
				}
				//spells loaded
				spec->defaultMianStats = bufor[i]; ++i;
				if(spec->defaultMianStats)
				{
					spec->attack = bufor[i]; ++i;
					spec->defence = bufor[i]; ++i;
					spec->power = bufor[i]; ++i;
					spec->knowledge = bufor[i]; ++i;
				}
				i+=16;
				nobj->info = spec;
				//////creating CHeroInstance
				CHeroInstance * nhi = new CHeroInstance;
				nhi->exp = spec->experience;
				nhi->level = CGI->heroh->level(nhi->exp);
				nhi->mana = spec->knowledge * 10;
				nhi->movement = -1;
				nhi->name = spec->name;
				nhi->owner = spec->player;
				nhi->pos = nobj->pos;
				nhi->type = spec->type;
				nhi->army = spec->garrison;
				CGI->heroh->heroInstances.push_back(nhi);
				break;
			}
		case CREATURES_DEF:
			{
				CCreatureObjInfo * spec = new CCreatureObjInfo;
				spec->bytes[0] = bufor[i]; ++i;
				spec->bytes[1] = bufor[i]; ++i;
				spec->bytes[2] = bufor[i]; ++i;
				spec->bytes[3] = bufor[i]; ++i;
				spec->number = readNormalNr(i, 2); i+=2;
				spec->character = bufor[i]; ++i;
				bool isMesTre = bufor[i]; ++i; //true if there is message or treasury
				if(isMesTre)
				{
					int messLength = readNormalNr(i); i+=4;
					if(messLength>0)
					{
						for(int tt=0; tt<messLength; ++tt)
						{
							spec->message += bufor[i]; ++i;
						}
					}
					spec->wood = readNormalNr(i); i+=4;
					spec->mercury = readNormalNr(i); i+=4;
					spec->ore = readNormalNr(i); i+=4;
					spec->sulfur = readNormalNr(i); i+=4;
					spec->crytal = readNormalNr(i); i+=4;
					spec->gems = readNormalNr(i); i+=4;
					spec->gold = readNormalNr(i); i+=4;
					int artID = readNormalNr(i, 2); i+=2;
					if(artID!=0xffff)
						spec->gainedArtifact = &(CGameInfo::mainObj->arth->artifacts[artID]);
					else
						spec->gainedArtifact = NULL;
				}
				spec->neverFlees = bufor[i]; ++i;
				spec->notGrowingTeam = bufor[i]; ++i;
				i+=2;
				nobj->info = spec;
				break;
			}
		case EDefType::SIGN_DEF:
			{
				CSignObjInfo * spec = new CSignObjInfo;
				int length = readNormalNr(i); i+=4;
				for(int rr=0; rr<length; ++rr)
				{
					spec->message += bufor[i]; ++i;
				}
				i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::SEERHUT_DEF:
			{
				CSeerHutObjInfo * spec = new CSeerHutObjInfo;
				spec->missionType = bufor[i]; ++i;
				switch(spec->missionType)
				{
				case 0:
					i+=3;
					continue;
				case 1:
					{
						spec->m1level = readNormalNr(i); i+=4;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 2:
					{
						spec->m2attack = bufor[i]; ++i;
						spec->m2defence = bufor[i]; ++i;
						spec->m2power = bufor[i]; ++i;
						spec->m2knowledge = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 3:
					{
						spec->m3bytes[0] = bufor[i]; ++i;
						spec->m3bytes[1] = bufor[i]; ++i;
						spec->m3bytes[2] = bufor[i]; ++i;
						spec->m3bytes[3] = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 4:
					{
						spec->m4bytes[0] = bufor[i]; ++i;
						spec->m4bytes[1] = bufor[i]; ++i;
						spec->m4bytes[2] = bufor[i]; ++i;
						spec->m4bytes[3] = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 5:
					{
						int artNumber = bufor[i]; ++i;
						for(int yy=0; yy<artNumber; ++yy)
						{
							int artid = readNormalNr(i, 2); i+=2;
							spec->m5arts.push_back(&(CGameInfo::mainObj->arth->artifacts[artid]));
						}
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 6:
					{
						int typeNumber = bufor[i]; ++i;
						for(int hh=0; hh<typeNumber; ++hh)
						{
							int creType = readNormalNr(i, 2); i+=2;
							int creNumb = readNormalNr(i, 2); i+=2;
							spec->m6cre.push_back(&(CGameInfo::mainObj->creh->creatures[creType]));
							spec->m6number.push_back(creNumb);
						}
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 7:
					{
						spec->m7wood = readNormalNr(i); i+=4;
						spec->m7mercury = readNormalNr(i); i+=4;
						spec->m7ore = readNormalNr(i); i+=4;
						spec->m7sulfur = readNormalNr(i); i+=4;
						spec->m7crystal = readNormalNr(i); i+=4;
						spec->m7gems = readNormalNr(i); i+=4;
						spec->m7gold = readNormalNr(i); i+=4;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 8:
					{
						int heroType = bufor[i]; ++i;
						spec->m8hero = CGameInfo::mainObj->heroh->heroes[heroType];
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 9:
					{
						spec->m9player = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				}//internal switch end (seer huts)

				int len1 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len1; ++ee)
				{
					spec->firstVisitText += bufor[i]; ++i;
				}

				int len2 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len2; ++ee)
				{
					spec->nextVisitText += bufor[i]; ++i;
				}

				int len3 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len3; ++ee)
				{
					spec->completedText += bufor[i]; ++i;
				}

				unsigned char rewardType = bufor[i]; ++i;
				spec->rewardType = rewardType;

				switch(rewardType)
				{
				case 1:
					{
						spec->r1exp = readNormalNr(i); i+=4;
						break;
					}
				case 2:
					{
						spec->r2mana = readNormalNr(i); i+=4;
						break;
					}
				case 3:
					{
						spec->r3morale = bufor[i]; ++i;
						break;
					}
				case 4:
					{
						spec->r4luck = bufor[i]; ++i;
						break;
					}
				case 5:
					{
						spec->r5type = bufor[i]; ++i;
						spec->r5amount = readNormalNr(i, 3); i+=3;
						i+=1;
						break;
					}
				case 6:
					{
						spec->r6type = bufor[i]; ++i;
						spec->r6amount = bufor[i]; ++i;
						break;
					}
				case 7:
					{
						int abid = bufor[i]; ++i;
						spec->r7ability = CGameInfo::mainObj->abilh->abilities[abid];
						spec->r7level = bufor[i]; ++i;
						break;
					}
				case 8:
					{
						int artid = readNormalNr(i, 2); i+=2;
						spec->r8art = &(CGameInfo::mainObj->arth->artifacts[artid]);
						break;
					}
				case 9:
					{
						int spellid = bufor[i]; ++i;
						spec->r9spell = &(CGameInfo::mainObj->spellh->spells[spellid]);
						break;
					}
				case 10:
					{
						int creid = readNormalNr(i, 2); i+=2;
						spec->r10creature = &(CGameInfo::mainObj->creh->creatures[creid]);
						spec->r10amount = readNormalNr(i, 2); i+=2;
						break;
					}
				}// end of internal switch
				i+=2;
				nobj->info = spec;
				break;
			}
		case EDefType::WITCHHUT_DEF:
			{
				CWitchHutObjInfo * spec = new CWitchHutObjInfo;
				ist=i; //starting i for loop
				for(i; i<ist+4; ++i)
				{
					unsigned char c = bufor[i];
					for(int yy=0; yy<8; ++yy)
					{
						if((i-ist)*8+yy < CGameInfo::mainObj->abilh->abilities.size())
						{
							if(c == (c|((unsigned char)intPow(2, yy))))
								spec->allowedAbilities.push_back(CGameInfo::mainObj->abilh->abilities[(i-ist)*8+yy]);
						}
					}
				}
				
				nobj->info = spec;
				break;
			}
		case EDefType::SCHOLAR_DEF:
			{
				CScholarObjInfo * spec = new CScholarObjInfo;
				spec->bonusType = bufor[i]; ++i;
				switch(spec->bonusType)
				{
				case 0xff:
					++i;
					break;
				case 0:
					spec->r0type = bufor[i]; ++i;
					break;
				case 1:
					spec->r1 = CGameInfo::mainObj->abilh->abilities[bufor[i]]; ++i;
					break;
				case 2:
					spec->r2 = &(CGameInfo::mainObj->spellh->spells[bufor[i]]); ++i;
					break;
				}
				i+=6;
				nobj->info = spec;
				break;
			}
		case EDefType::GARRISON_DEF:
			{
				CGarrisonObjInfo * spec = new CGarrisonObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->units = readCreatureSet(i); i+=28;
				spec->movableUnits = bufor[i]; ++i;
				i+=8;
				nobj->info = spec;
				break;
			}
		case EDefType::ARTIFACT_DEF:
			{
				CArtifactObjInfo * spec = new CArtifactObjInfo;
				bool areSettings = bufor[i]; ++i;
				if(areSettings)
				{
					int messLength = readNormalNr(i, 4); i+=4;
					for(int hh=0; hh<messLength; ++hh)
					{
						spec->message += bufor[i]; ++i;
					}
					bool areGuards = bufor[i]; ++i;
					if(areGuards)
					{
						spec->areGuards = true;
						spec->guards = readCreatureSet(i); i+=28;
					}
					else
						spec->areGuards = false;
					i+=4;
				}
				nobj->info = spec;
				break;
			}
		case EDefType::RESOURCE_DEF:
			{
				CResourceObjInfo * spec = new CResourceObjInfo;
				bool isMessGuard = bufor[i]; ++i;
				if(isMessGuard)
				{
					int messLength = readNormalNr(i); i+=4;
					for(int mm=0; mm<messLength; ++mm)
					{
						spec->message+=bufor[i]; ++i;
					}
					spec->areGuards = bufor[i]; ++i;
					if(spec->areGuards)
					{
						spec->guards = readCreatureSet(i); i+=28;
					}
					i+=4;
				}
				else
				{
					spec->areGuards = false;
				}
				spec->amount = readNormalNr(i); i+=4;
				i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::TOWN_DEF:
			{
				CCastleObjInfo * spec = new CCastleObjInfo;
				if(map.version!=RoE)
				{
					spec->bytes[0] = bufor[i]; ++i;
					spec->bytes[1] = bufor[i]; ++i;
					spec->bytes[2] = bufor[i]; ++i;
					spec->bytes[3] = bufor[i]; ++i;
				}
				else
				{
					spec->bytes[0] = 0;
					spec->bytes[1] = 0;
					spec->bytes[2] = 0;
					spec->bytes[3] = 0;
				}
				spec->player = bufor[i]; ++i;

				bool hasName = bufor[i]; ++i;
				if(hasName)
				{
					int len = readNormalNr(i); i+=4;
					for(int gg=0; gg<len; ++gg)
					{
						spec->name += bufor[i]; ++i;
					}
				}
				bool stGarr = bufor[i]; ++i; //true if garrison isn't empty
				if(stGarr)
				{
					spec->garrison = readCreatureSet(i); i+=28;
				}
				spec->garrison.formation = bufor[i]; ++i;
				spec->unusualBuildins = bufor[i]; ++i;
				if(spec->unusualBuildins)
				{
					for(int ff=0; ff<12; ++ff)
					{
						spec->buildingSettings[ff] = bufor[i]; ++i;
					}
				}
				else
				{
					spec->hasFort = bufor[i]; ++i;
				}

				int ist = i;
				for(i; i<ist+9; ++i)
				{
					unsigned char c = bufor[i];
					for(int yy=0; yy<8; ++yy)
					{
						if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
						{
							if(c == (c|((unsigned char)intPow(2, yy))))
								spec->obligatorySpells.push_back(&(CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy]));
						}
					}
				}

				ist = i;
				for(i; i<ist+9; ++i)
				{
					unsigned char c = bufor[i];
					for(int yy=0; yy<8; ++yy)
					{
						if((i-ist)*8+yy < CGameInfo::mainObj->spellh->spells.size())
						{
							if(c != (c|((unsigned char)intPow(2, yy))))
								spec->possibleSpells.push_back(&(CGameInfo::mainObj->spellh->spells[(i-ist)*8+yy]));
						}
					}
				}

				/////// reading castle events //////////////////////////////////

				int numberOfEvent = readNormalNr(i); i+=4;

				for(int gh = 0; gh<numberOfEvent; ++gh)
				{
					CCastleEvent nce;
					int nameLen = readNormalNr(i); i+=4;
					for(int ll=0; ll<nameLen; ++ll)
					{
						nce.name += bufor[i]; ++i;
					}

					int messLen = readNormalNr(i); i+=4;
					for(int ll=0; ll<messLen; ++ll)
					{
						nce.message += bufor[i]; ++i;
					}

					nce.wood = readNormalNr(i); i+=4;
					nce.mercury = readNormalNr(i); i+=4;
					nce.ore = readNormalNr(i); i+=4;
					nce.sulfur = readNormalNr(i); i+=4;
					nce.crystal = readNormalNr(i); i+=4;
					nce.gems = readNormalNr(i); i+=4;
					nce.gold = readNormalNr(i); i+=4;

					nce.players = bufor[i]; ++i;
					nce.forHuman = bufor[i]; ++i;
					nce.forComputer = bufor[i]; ++i;
					nce.firstShow = readNormalNr(i, 2); i+=2;
					nce.forEvery = bufor[i]; ++i;

					i+=17;

					for(int kk=0; kk<6; ++kk)
					{
						nce.bytes[kk] = bufor[i]; ++i;
					}

					for(int vv=0; vv<7; ++vv)
					{
						nce.gen[vv] = readNormalNr(i, 2); i+=2;
					}
					i+=4;
					spec->events.push_back(nce);
				}

				/////// castle events have been read ///////////////////////////

				spec->alignment = bufor[i]; ++i;
				i+=3;
				nobj->info = spec;
				break;
			}
		case EDefType::PLAYERONLY_DEF:
			{
				CPlayerOnlyObjInfo * spec = new CPlayerOnlyObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				nobj->info = spec;
				break;
			}
		case EDefType::SHRINE_DEF:
			{
				CShrineObjInfo * spec = new CShrineObjInfo;
				spec->spell = bufor[i]; i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::SPELLSCROLL_DEF:
			{
				CSpellScrollObjinfo * spec = new CSpellScrollObjinfo;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					int mLength = readNormalNr(i); i+=4;
					for(int vv=0; vv<mLength; ++vv)
					{
						spec->message += bufor[i]; ++i;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(i); i+=28;
					}
					i+=4;
				}
				spec->spell = &(CGameInfo::mainObj->spellh->spells[bufor[i]]); ++i;
				i+=3;
				nobj->info = spec;
				break;
			}
		case EDefType::PANDORA_DEF:
			{
				CPandorasBoxObjInfo * spec = new CPandorasBoxObjInfo;
				bool messg = bufor[i]; ++i;
				if(messg)
				{
					int mLength = readNormalNr(i); i+=4;
					for(int vv=0; vv<mLength; ++vv)
					{
						spec->message += bufor[i]; ++i;
					}
					spec->areGuarders = bufor[i]; ++i;
					if(spec->areGuarders)
					{
						spec->guarders = readCreatureSet(i); i+=28;
					}
					i+=4;
				}
				////// copied form event handling (seems to be similar)
				spec->gainedExp = readNormalNr(i, 4); i+=4;
				spec->manaDiff = readNormalNr(i, 4); i+=4;
				spec->moraleDiff = readNormalNr(i, 1, true); ++i;
				spec->luckDiff = readNormalNr(i, 1, true); ++i;
				spec->wood = readNormalNr(i); i+=4;
				spec->mercury = readNormalNr(i); i+=4;
				spec->ore = readNormalNr(i); i+=4;
				spec->sulfur = readNormalNr(i); i+=4;
				spec->crystal = readNormalNr(i); i+=4;
				spec->gems = readNormalNr(i); i+=4;
				spec->gold = readNormalNr(i); i+=4;
				spec->attack = readNormalNr(i, 1); ++i;
				spec->defence = readNormalNr(i, 1); ++i;
				spec->power = readNormalNr(i, 1); ++i;
				spec->knowledge = readNormalNr(i, 1); ++i;
				int gabn; //number of gained abilities
				gabn = readNormalNr(i, 1); ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
					spec->abilities.push_back((CGameInfo::mainObj->abilh)->abilities[readNormalNr(i, 1)]); ++i;
					spec->abilityLevels.push_back(readNormalNr(i, 1)); ++i;
				}
				int gart = readNormalNr(i, 1); ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
					spec->artifacts.push_back(&(CGameInfo::mainObj->arth->artifacts[readNormalNr(i, 2)])); i+=2;
				}
				int gspel = readNormalNr(i, 1); ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
					spec->spells.push_back(&(CGameInfo::mainObj->spellh->spells[readNormalNr(i, 1)])); ++i;
				}
				int gcre = readNormalNr(i, 1); ++i; //number of gained creatures
				spec->creatures = readCreatureSet(i, gcre); i+=4*gcre;
				i+=8;
				nobj->info = spec;
				///////end of copied fragment
				break;
			}
		case EDefType::GRAIL_DEF:
			{
				CGrailObjInfo * spec = new CGrailObjInfo;
				spec->radius = readNormalNr(i); i+=4;
				nobj->info = spec;
				break;
			}
		case EDefType::CREGEN_DEF:
			{
				CCreGenObjInfo * spec = new CCreGenObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				for(int ggg=0; ggg<4; ++ggg)
				{
					spec->bytes[ggg] = bufor[i]; ++i;
				}
				if((spec->bytes[0] == '\0') && (spec->bytes[1] == '\0') && (spec->bytes[2] == '\0') && (spec->bytes[3] == '\0'))
				{
					spec->asCastle = false;
					spec->castles[0] = bufor[i]; ++i;
					spec->castles[1] = bufor[i]; ++i;
				}
				else
				{
					spec->asCastle = true;
				}
				nobj->info = spec;
				break;
			}
		case EDefType::CREGEN2_DEF:
			{
				CCreGen2ObjInfo * spec = new CCreGen2ObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				for(int ggg=0; ggg<4; ++ggg)
				{
					spec->bytes[ggg] = bufor[i]; ++i;
				}
				if((spec->bytes[0] == '\0') && (spec->bytes[1] == '\0') && (spec->bytes[2] == '\0') && (spec->bytes[3] == '\0'))
				{
					spec->asCastle = false;
					spec->castles[0] = bufor[i]; ++i;
					spec->castles[1] = bufor[i]; ++i;
				}
				else
				{
					spec->asCastle = true;
				}
				spec->minLevel = bufor[i]; ++i;
				spec->maxLevel = bufor[i]; ++i;
				if(spec->maxLevel>7)
					spec->maxLevel = 7;
				if(spec->minLevel<1)
					spec->minLevel = 1;
				nobj->info = spec;
				break;
			}
		case EDefType::CREGEN3_DEF:
			{
				CCreGen3ObjInfo * spec = new CCreGen3ObjInfo;
				spec->player = bufor[i]; ++i;
				i+=3;
				spec->minLevel = bufor[i]; ++i;
				spec->maxLevel = bufor[i]; ++i;
				if(spec->maxLevel>7)
					spec->maxLevel = 7;
				if(spec->minLevel<1)
					spec->minLevel = 1;
				nobj->info = spec;
				break;
			}
		case EDefType::BORDERGUARD_DEF:
			{
				CBorderGuardObjInfo * spec = new CBorderGuardObjInfo;
				spec->missionType = bufor[i]; ++i;
				switch(spec->missionType)
				{
				case 1:
					{
						spec->m1level = readNormalNr(i); i+=4;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 2:
					{
						spec->m2attack = bufor[i]; ++i;
						spec->m2defence = bufor[i]; ++i;
						spec->m2power = bufor[i]; ++i;
						spec->m2knowledge = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 3:
					{
						spec->m3bytes[0] = bufor[i]; ++i;
						spec->m3bytes[1] = bufor[i]; ++i;
						spec->m3bytes[2] = bufor[i]; ++i;
						spec->m3bytes[3] = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 4:
					{
						spec->m4bytes[0] = bufor[i]; ++i;
						spec->m4bytes[1] = bufor[i]; ++i;
						spec->m4bytes[2] = bufor[i]; ++i;
						spec->m4bytes[3] = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 5:
					{
						int artNumber = bufor[i]; ++i;
						for(int yy=0; yy<artNumber; ++yy)
						{
							int artid = readNormalNr(i, 2); i+=2;
							spec->m5arts.push_back(&(CGameInfo::mainObj->arth->artifacts[artid]));
						}
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 6:
					{
						int typeNumber = bufor[i]; ++i;
						for(int hh=0; hh<typeNumber; ++hh)
						{
							int creType = readNormalNr(i, 2); i+=2;
							int creNumb = readNormalNr(i, 2); i+=2;
							spec->m6cre.push_back(&(CGameInfo::mainObj->creh->creatures[creType]));
							spec->m6number.push_back(creNumb);
						}
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 7:
					{
						spec->m7wood = readNormalNr(i); i+=4;
						spec->m7mercury = readNormalNr(i); i+=4;
						spec->m7ore = readNormalNr(i); i+=4;
						spec->m7sulfur = readNormalNr(i); i+=4;
						spec->m7crystal = readNormalNr(i); i+=4;
						spec->m7gems = readNormalNr(i); i+=4;
						spec->m7gold = readNormalNr(i); i+=4;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 8:
					{
						int heroType = bufor[i]; ++i;
						spec->m8hero = CGameInfo::mainObj->heroh->heroes[heroType];
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				case 9:
					{
						spec->m9player = bufor[i]; ++i;
						int limit = readNormalNr(i); i+=4;
						if(limit == ((int)0xffffffff))
						{
							spec->isDayLimit = false;
							spec->lastDay = -1;
						}
						else
						{
							spec->isDayLimit = true;
							spec->lastDay = limit;
						}
						break;
					}
				}//internal switch end (seer huts)

				int len1 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len1; ++ee)
				{
					spec->firstVisitText += bufor[i]; ++i;
				}

				int len2 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len2; ++ee)
				{
					spec->nextVisitText += bufor[i]; ++i;
				}

				int len3 = readNormalNr(i); i+=4;
				for(int ee=0; ee<len3; ++ee)
				{
					spec->completedText += bufor[i]; ++i;
				}
				nobj->info = spec;
				break;
			}
		} //end of main switch
		CGameInfo::mainObj->objh->objInstances.push_back(nobj);
		//TODO - doko�czy�, du�o do zrobienia - trzeba patrze�, co def niesie
	}//*/ //end of loading objects; commented to make application work until it will be finished
	////objects loaded

	processMap(defsToUnpack);
	std::vector<CDefHandler *> dhandlers = CGameInfo::mainObj->spriteh->extractManyFiles(defsToUnpack);
	for (int i=0;i<dhandlers.size();i++)
		map.defy[i].handler=dhandlers[i];
	for(int vv=0; vv<map.defy.size(); ++vv)
	{
		if(map.defy[vv].handler->alphaTransformed)
			continue;
		for(int yy=0; yy<map.defy[vv].handler->ourImages.size(); ++yy)
		{
			map.defy[vv].handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(map.defy[vv].handler->ourImages[yy].bitmap);
			SDL_Surface * bufs = CSDL_Ext::secondAlphaTransform(map.defy[vv].handler->ourImages[yy].bitmap, alphaTransSurf);
			SDL_FreeSurface(map.defy[vv].handler->ourImages[yy].bitmap);
			map.defy[vv].handler->ourImages[yy].bitmap = bufs;
			map.defy[vv].handler->alphaTransformed = true;
		}
	}
	SDL_FreeSurface(alphaTransSurf);

	//assigning defobjinfos

	for(int ww=0; ww<CGI->objh->objInstances.size(); ++ww)
	{
		for(int h=0; h<CGI->dobjinfo->objs.size(); ++h)
		{
			if(CGI->dobjinfo->objs[h].defName==CGI->ac->map.defy[CGI->objh->objInstances[ww]->defNumber].name)
			{
				CGI->objh->objInstances[ww]->defObjInfoNumber = h;
				break;
			}
		}
	}

	//assigned

	//loading events
	int numberOfEvents = readNormalNr(i); i+=4;
	for(int yyoo=0; yyoo<numberOfEvents; ++yyoo)
	{
		CMapEvent ne;
		ne.name = std::string();
		ne.message = std::string();
		int nameLen = readNormalNr(i); i+=4;
		for(int qq=0; qq<nameLen; ++qq)
		{
			ne.name += bufor[i]; ++i;
		}
		int messLen = readNormalNr(i); i+=4;
		for(int qq=0; qq<messLen; ++qq)
		{
			ne.message +=bufor[i]; ++i;
		}
		ne.wood = readNormalNr(i); i+=4;
		ne.mercury = readNormalNr(i); i+=4;
		ne.ore = readNormalNr(i); i+=4;
		ne.sulfur = readNormalNr(i); i+=4;
		ne.crystal = readNormalNr(i); i+=4;
		ne.gems = readNormalNr(i); i+=4;
		ne.gold = readNormalNr(i); i+=4;
		ne.players = bufor[i]; ++i;
		ne.humanAffected = bufor[i]; ++i;
		ne.computerAffected = bufor[i]; ++i;
		ne.firstOccurence = bufor[i]; ++i;
		ne.nextOccurence = bufor[i]; ++i;
		i+=18;
		map.events.push_back(ne);
	}
}
int CAmbarCendamo::readNormalNr (int pos, int bytCon, bool cyclic)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[pos+i]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}

void CAmbarCendamo::loadDefs()
{
	std::set<int> loadedTypes;
	for (int i=0; i<map.width; i++)
	{
		for (int j=0; j<map.width; j++)
		{
			if (loadedTypes.find(map.terrain[i][j].tertype)==loadedTypes.end())
			{
				CDefHandler  *sdh = CGI->spriteh->giveDef(CSemiDefHandler::nameFromType(map.terrain[i][j].tertype).c_str());
				loadedTypes.insert(map.terrain[i][j].tertype);
				defs.push_back(sdh);
			}
			if (map.twoLevel && loadedTypes.find(map.undergroungTerrain[i][j].tertype)==loadedTypes.end())
			{
				CDefHandler  *sdh = CGI->spriteh->giveDef(CSemiDefHandler::nameFromType(map.undergroungTerrain[i][j].tertype).c_str());
				loadedTypes.insert(map.undergroungTerrain[i][j].tertype);
				defs.push_back(sdh);
			}
		}
	}
}

EDefType CAmbarCendamo::getDefType(DefInfo &a)
{
	switch(a.bytes[16])
	{
	case 5: case 65: case 66: case 67: case 68: case 69:
		return EDefType::ARTIFACT_DEF; //handled
	case 6:
		return EDefType::PANDORA_DEF; //hanled
	case 26:
		return EDefType::EVENTOBJ_DEF; //handled
	case 33:
		return EDefType::GARRISON_DEF; //handled
	case 34: case 70: case 62: //70 - random hero //62 - prison
		return EDefType::HERO_DEF; //handled
	case 36:
		return EDefType::GRAIL_DEF; //hanled
	case 53: case 17: case 18: case 19: case 20: case 42: case 87: case 220://cases 17 - 20 and 42 - tests
		return EDefType::PLAYERONLY_DEF; //handled
	case 54: case 71: case 72: case 73: case 74: case 75: case 162: case 163: case 164:
		return EDefType::CREATURES_DEF; //handled
	case 59:
		return EDefType::SIGN_DEF; //handled
	case 77:
		return EDefType::TOWN_DEF; //can be problematic, but handled
	case 79: case 76:
		return EDefType::RESOURCE_DEF; //handled
	case 81:
		return EDefType::SCHOLAR_DEF; //handled
	case 83:
		return EDefType::SEERHUT_DEF; //handled
	case 91:
		return EDefType::SIGN_DEF; //handled
	case 88: case 89: case 90:
		return SHRINE_DEF; //handled
	case 93:
		return SPELLSCROLL_DEF; //handled
	case 98:
		return EDefType::TOWN_DEF; //handled
	case 113:
		return EDefType::WITCHHUT_DEF; //handled
	case 215:
		return EDefType::BORDERGUARD_DEF; //handled by analogy to seer huts ;]
	case 216:
		return EDefType::CREGEN2_DEF; //handled
	case 217:
		return EDefType::CREGEN_DEF; //handled
	case 218:
		return EDefType::CREGEN3_DEF; //handled
	case 219:
		return EDefType::GARRISON_DEF; //handled
	default:
		return EDefType::TERRAINOBJ_DEF; // nothing to be handled
	}
}

CCreatureSet CAmbarCendamo::readCreatureSet(int pos, int number)
{
	CCreatureSet ret;
	std::pair<CCreature *, int> ins;
	if(number>0 && readNormalNr(pos, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+2, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(0,ins);
		ret.slots.insert(tt);
	}
	if(number>1 && readNormalNr(pos+4, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+4, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+6, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(1,ins);
		ret.slots.insert(tt);
	}
	if(number>2 && readNormalNr(pos+8, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+8, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+10, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(2,ins);
		ret.slots.insert(tt);
	}
	if(number>3 && readNormalNr(pos+12, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+12, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+14, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(3,ins);
		ret.slots.insert(tt);
	}
	if(number>4 && readNormalNr(pos+16, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+16, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+18, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(4,ins);
		ret.slots.insert(tt);
	}
	if(number>5 && readNormalNr(pos+20, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+20, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+22, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(5,ins);
		ret.slots.insert(tt);
	}
	if(number>6 && readNormalNr(pos+24, 2)!=0xffff)
	{
		int rettt = readNormalNr(pos+24, 2);
		if(rettt>32768)
			rettt = 65536-rettt+CGameInfo::mainObj->creh->creatures.size()-16;
		ins.first  = &(CGameInfo::mainObj->creh->creatures[rettt]);
		ins.second = readNormalNr(pos+26, 2);
		std::pair<int,std::pair<CCreature *, int> > tt(6,ins);
		ret.slots.insert(tt);
	}
	return ret;
}

void CAmbarCendamo::processMap(std::vector<std::string> & defsToUnpack)
{
	resDefNames.push_back("AVTRNDM0.DEF");
	resDefNames.push_back("AVTWOOD0.DEF");
	resDefNames.push_back("AVTMERC0.DEF");
	resDefNames.push_back("AVTORE0.DEF");
	resDefNames.push_back("AVTSULF0.DEF");
	resDefNames.push_back("AVTCRYS0.DEF");
	resDefNames.push_back("AVTGEMS0.DEF");
	resDefNames.push_back("AVTGOLD0.DEF");
	resDefNames.push_back("ZMITHR.DEF");

	std::vector<int> resDefNumbers;
	for(int hh=0; hh<resDefNames.size(); ++hh)
	{
		resDefNumbers.push_back(-1);
		for(int k=0; k<map.defy.size(); ++k)
		{
			std::string buf = map.defy[k].name;
			std::transform(buf.begin(), buf.end(), buf.begin(), (int(*)(int))toupper);
			if(resDefNames[hh] == buf)
			{
				resDefNumbers[resDefNumbers.size()-1] = k;
				break;
			}
		}
	}
	std::vector<std::string> creDefNames;
	for(int dd=0; dd<140; ++dd) //we do not use here WoG units
	{
		creDefNames.push_back(CGI->dobjinfo->objs[dd+1184].defName);
	}
	std::vector<int> creDefNumbers;
	for(int ee=0; ee<creDefNames.size(); ++ee)
	{
		creDefNumbers.push_back(-1);
	}
	std::vector<std::string> artDefNames;
	std::vector<int> artDefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass!=EartClass::SartClass)
		artDefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<artDefNames.size(); ++ee)
	{
		artDefNumbers.push_back(-1);
	}
	
	std::vector<std::string> art1DefNames;
	std::vector<int> art1DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::TartClass)
			art1DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art1DefNames.size(); ++ee)
	{
		art1DefNumbers.push_back(-1);
	}
	
	std::vector<std::string> art2DefNames;
	std::vector<int> art2DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::NartClass)
			art2DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art2DefNames.size(); ++ee)
	{
		art2DefNumbers.push_back(-1);
	}

	std::vector<std::string> art3DefNames;
	std::vector<int> art3DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::JartClass)
			art3DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art3DefNames.size(); ++ee)
	{
		art3DefNumbers.push_back(-1);
	}

	std::vector<std::string> art4DefNames;
	std::vector<int> art4DefNumbers;
	for(int bb=0; bb<162; ++bb)
	{
		if(CGI->arth->artifacts[CGI->dobjinfo->objs[bb+213].subtype].aClass==EartClass::RartClass)
			art4DefNames.push_back(CGI->dobjinfo->objs[bb+213].defName);
	}
	for(int ee=0; ee<art4DefNames.size(); ++ee)
	{
		art4DefNumbers.push_back(-1);
	}

	std::vector<std::string> town0DefNames; //without fort
	std::vector<int> town0DefNumbers;
	std::vector<std::string> town1DefNames; //with fort
	std::vector<int> town1DefNumbers;

	for(int dd=0; dd<F_NUMBER; ++dd)
	{
		town1DefNames.push_back(CGI->dobjinfo->objs[dd+385].defName);
		town1DefNumbers.push_back(-1);
	}

	std::vector< std::vector<std::string> > creGenNames;
	std::vector< std::vector<int> > creGenNumbers;
	creGenNames.resize(F_NUMBER);
	creGenNumbers.resize(F_NUMBER);

	for(int ff=0; ff<F_NUMBER-1; ++ff)
	{
		for(int dd=0; dd<7; ++dd)
		{
			creGenNames[ff].push_back(CGI->dobjinfo->objs[395+7*ff+dd].defName);
			creGenNumbers[ff].push_back(-1);
		}
	}

	for(int dd=0; dd<7; ++dd)
	{
		creGenNumbers[8].push_back(-1);
	}

	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[457].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[452].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[455].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[454].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[453].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[458].defName);
	creGenNames[F_NUMBER-1].push_back(CGI->dobjinfo->objs[459].defName);

	for(int b=0; b<CGI->scenarioOps.playerInfos.size(); ++b)
	{
		if(CGI->scenarioOps.playerInfos[b].castle==-1)
			CGI->scenarioOps.playerInfos[b].castle = rand()%F_NUMBER;
	}
	
	//variables initialized
	for(int j=0; j<CGI->objh->objInstances.size(); ++j)
	{
		DefInfo curDef = map.defy[CGI->objh->objInstances[j]->defNumber];
		switch(getDefType(curDef))
		{
		case EDefType::RESOURCE_DEF:
			{
				if(curDef.bytes[16]==76) //resource to specify
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 79;
					nxt.bytes[20] = rand()%7;
					if(resDefNumbers[nxt.bytes[20]+1]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = resDefNumbers[nxt.bytes[20]+1];
						continue;
					}
					nxt.name = resDefNames[nxt.bytes[20]+1];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(resDefNumbers[nxt.bytes[20]+1]==-1)
					{
						resDefNumbers[nxt.bytes[20]+1] = map.defy.size()-1;
					}
				}
				break;
			}
		case EDefType::CREATURES_DEF:
			{
				if(curDef.bytes[16]==72) //random monster lvl 1
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==73) //random monster lvl 2
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2+2;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==74) //random monster lvl 3
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2+4;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==75) //random monster lvl 4
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2+6;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==162) //random monster lvl 5
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2+8;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==163) //random monster lvl 6
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2+10;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==164) //random monster lvl 7
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = 14*(rand()%9)+rand()%2+12;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==71) //random monster (any level)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 54;
					nxt.bytes[20] = rand()%126;
					if(creDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = creDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creDefNumbers[nxt.bytes[20]]==-1)
					{
						creDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				break;
			} //end of case
		case EDefType::ARTIFACT_DEF:
			{
				if(curDef.bytes[16]==65) //random atrifact (any class)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 5;
					nxt.bytes[20] = rand()%artDefNames.size();
					if(artDefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = artDefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = artDefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(artDefNumbers[nxt.bytes[20]]==-1)
					{
						artDefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==66) //random atrifact (treasure)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 5;
					nxt.bytes[20] = rand()%art1DefNames.size();
					if(art1DefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = art1DefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = art1DefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(art1DefNumbers[nxt.bytes[20]]==-1)
					{
						art1DefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==67) //random atrifact (minor)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 5;
					nxt.bytes[20] = rand()%art2DefNames.size();
					if(art2DefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = art2DefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = art2DefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(art2DefNumbers[nxt.bytes[20]]==-1)
					{
						art2DefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==68) //random atrifact (major)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 5;
					nxt.bytes[20] = rand()%art3DefNames.size();
					if(art3DefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = art3DefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = art3DefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(art3DefNumbers[nxt.bytes[20]]==-1)
					{
						art3DefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				if(curDef.bytes[16]==69) //random atrifact (relic)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 5;
					nxt.bytes[20] = rand()%art4DefNames.size();
					if(art4DefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = art4DefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = art4DefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(art4DefNumbers[nxt.bytes[20]]==-1)
					{
						art4DefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				break;
			}
		case EDefType::TOWN_DEF:
			{
				if(curDef.bytes[16]==77) //random town
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 98;
					if(((CCastleObjInfo*)CGI->objh->objInstances[j]->info)->player==0xff)
					{
						nxt.bytes[20] = rand()%town1DefNames.size();		
					}
					else
					{
						if(CGI->scenarioOps.getIthPlayersSettings(((CCastleObjInfo*)CGI->objh->objInstances[j]->info)->player).castle>-1)
						{
							nxt.bytes[20] = CGI->scenarioOps.getIthPlayersSettings(((CCastleObjInfo*)CGI->objh->objInstances[j]->info)->player).castle;
						}
						else
						{
							nxt.bytes[20] = rand()%town1DefNames.size();
						}
					}
					if(town1DefNumbers[nxt.bytes[20]]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = town1DefNumbers[nxt.bytes[20]];
						continue;
					}
					nxt.name = town1DefNames[nxt.bytes[20]];
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(town1DefNumbers[nxt.bytes[20]]==-1)
					{
						town1DefNumbers[nxt.bytes[20]] = map.defy.size()-1;
					}
				}
				//((CCastleObjInfo*)CGI->objh->objInstances[j].info)
				break;
			}
		} //end of main switch
	} //end of main loop
	for(int j=0; j<CGI->objh->objInstances.size(); ++j) //for creature dwellings on map (they are town type dependent)
	{
		DefInfo curDef = map.defy[CGI->objh->objInstances[j]->defNumber];
		switch(getDefType(curDef))
		{
		case EDefType::CREGEN_DEF:
			{
				if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->asCastle)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 17;
					for(int vv=0; vv<CGI->objh->objInstances.size(); ++vv)
					{
						if(getDefType(map.defy[CGI->objh->objInstances[vv]->defNumber])==EDefType::TOWN_DEF)
						{
							if(
								((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[0]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[0]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[1]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[1]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[2]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[2]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[3]==((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->bytes[3])
							{
								for(int mm=0; mm<town1DefNames.size(); ++mm)
								{
									std::transform(town1DefNames[mm].begin(), town1DefNames[mm].end(), town1DefNames[mm].begin(), (int(*)(int))toupper);
									std::string hlp = map.defy[CGI->objh->objInstances[vv]->defNumber].name;
									std::transform(hlp.begin(), hlp.end(), hlp.begin(), (int(*)(int))toupper);
									if(town1DefNames[mm]==hlp)
									{
										nxt.bytes[20] = mm;
									}
								}
							}
						}
					}
					int lvl = atoi(map.defy[CGI->objh->objInstances[j]->defNumber].name.substr(7, 8).c_str())-1;
					nxt.name = creGenNames[nxt.bytes[20]][lvl];
					if(creGenNumbers[nxt.bytes[20]][lvl]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creGenNumbers[nxt.bytes[20]][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creGenNumbers[nxt.bytes[20]][lvl]==-1)
					{
						creGenNumbers[nxt.bytes[20]][lvl] = map.defy.size()-1;
					}
				}
				else //if not as castle
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 17;
					std::vector<int> possibleTowns;
					for(int bb=0; bb<8; ++bb)
					{
						if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->castles[0] & (1<<bb))
						{
							possibleTowns.push_back(bb);
						}
					}
					if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->castles[1])
						possibleTowns.push_back(8);
					nxt.bytes[20] = possibleTowns[rand()%possibleTowns.size()];
					int lvl = atoi(map.defy[CGI->objh->objInstances[j]->defNumber].name.substr(7, 8).c_str())-1;
					nxt.name = creGenNames[nxt.bytes[20]][lvl];
					if(creGenNumbers[nxt.bytes[20]][lvl]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creGenNumbers[nxt.bytes[20]][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creGenNumbers[nxt.bytes[20]][lvl]==-1)
					{
						creGenNumbers[nxt.bytes[20]][lvl] = map.defy.size()-1;
					}
				}
				break;
			}
		case EDefType::CREGEN2_DEF:
			{
				if(((CCreGenObjInfo*)CGI->objh->objInstances[j]->info)->asCastle)
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 17;
					for(int vv=0; vv<CGI->objh->objInstances.size(); ++vv)
					{
						if(getDefType(map.defy[CGI->objh->objInstances[vv]->defNumber])==EDefType::TOWN_DEF)
						{
							if(
								((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[0]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[0]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[1]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[1]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[2]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[2]
							&&  ((CCastleObjInfo*)CGI->objh->objInstances[vv]->info)->bytes[3]==((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->bytes[3])
							{
								for(int mm=0; mm<town1DefNames.size(); ++mm)
								{
									std::transform(town1DefNames[mm].begin(), town1DefNames[mm].end(), town1DefNames[mm].begin(), (int(*)(int))toupper);
									std::string hlp = map.defy[CGI->objh->objInstances[vv]->defNumber].name;
									std::transform(hlp.begin(), hlp.end(), hlp.begin(), (int(*)(int))toupper);
									if(town1DefNames[mm]==hlp)
									{
										nxt.bytes[20] = mm;
									}
								}
							}
						}
					}
					int lvl;
					if((((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel)!=0) 
						lvl = rand()%(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel) + ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel;
					else lvl = 0;
					nxt.name = creGenNames[nxt.bytes[20]][lvl];
					if(creGenNumbers[nxt.bytes[20]][lvl]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creGenNumbers[nxt.bytes[20]][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creGenNumbers[nxt.bytes[20]][lvl]==-1)
					{
						creGenNumbers[nxt.bytes[20]][lvl] = map.defy.size()-1;
					}
				}
				else //if not as castle
				{
					DefInfo nxt = curDef;
					nxt.bytes[16] = 17;
					std::vector<int> possibleTowns;
					for(int bb=0; bb<8; ++bb)
					{
						if(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->castles[0] & (1<<bb))
						{
							possibleTowns.push_back(bb);
						}
					}
					if(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->castles[1])
						possibleTowns.push_back(8);
					nxt.bytes[20] = possibleTowns[rand()%possibleTowns.size()];
					int lvl = rand()%(((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel) + ((CCreGen2ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel;
					nxt.name = creGenNames[nxt.bytes[20]][lvl];
					if(creGenNumbers[nxt.bytes[20]][lvl]!=-1)
					{
						CGI->objh->objInstances[j]->defNumber = creGenNumbers[nxt.bytes[20]][lvl];
						continue;
					}
					std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
						nxt.name);
					if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
					{
						nxt.isOnDefList = false;
					}
					else
					{
						nxt.printPriority = pit->priority;
						nxt.isOnDefList = true;
					}
					map.defy.push_back(nxt); // add this def to the vector
					defsToUnpack.push_back(nxt.name);
					CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
					if(creGenNumbers[nxt.bytes[20]][lvl]==-1)
					{
						creGenNumbers[nxt.bytes[20]][lvl] = map.defy.size()-1;
					}
				}
			}
		case EDefType::CREGEN3_DEF:
			{
				DefInfo nxt = curDef;
				nxt.bytes[16] = 17;
				nxt.bytes[20] = atoi(map.defy[CGI->objh->objInstances[j]->defNumber].name.substr(7, 8).c_str());
				int lvl = -1;
				CCreGen3ObjInfo * ct = (CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info;
				if(ct->maxLevel>7)
					ct->maxLevel = 7;
				if(ct->minLevel<1)
					ct->minLevel = 1;
				if((((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel)!=0)
					lvl = rand()%(((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel - ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel) + ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->minLevel;
				else
					lvl = ((CCreGen3ObjInfo*)CGI->objh->objInstances[j]->info)->maxLevel;
				nxt.name = creGenNames[nxt.bytes[20]][lvl];
				if(creGenNumbers[nxt.bytes[20]][lvl]!=-1)
				{
					CGI->objh->objInstances[j]->defNumber = creGenNumbers[nxt.bytes[20]][lvl];
					continue;
				}
				std::vector<DefObjInfo>::iterator pit = std::find(CGameInfo::mainObj->dobjinfo->objs.begin(), CGameInfo::mainObj->dobjinfo->objs.end(), 
					nxt.name);
				if(pit == CGameInfo::mainObj->dobjinfo->objs.end())
				{
					nxt.isOnDefList = false;
				}
				else
				{
					nxt.printPriority = pit->priority;
					nxt.isOnDefList = true;
				}
				map.defy.push_back(nxt); // add this def to the vector
				defsToUnpack.push_back(nxt.name);
				CGI->objh->objInstances[j]->defNumber = map.defy.size()-1;
				if(creGenNumbers[nxt.bytes[20]][lvl]==-1)
				{
					creGenNumbers[nxt.bytes[20]][lvl] = map.defy.size()-1;
				}
				break;
			}
		}//end of main switch
	} //end of sencond for loop
}
