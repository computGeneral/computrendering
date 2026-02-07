#include "cmMduBase.h"
#include "StatisticsManager.h"

#include <ostream.h>

/*
 * Ejemplo del registro de estadisticas en una mdu 
 * Tb se muestra el sistema autom�tico de registros de las boxes... ( dumpNameBoxes() )
 *
 * Nota, para que no salgan mensajes infomativos -> comentar el define #define GPU_MESSAGES
 */
class Box1 : public cmoMduBase {
private:

	U32& nMisses;
	U32& anotherStat;


public:	
	// las estadisticas se registran en la initializer list del contructor
	Box1( const char* name, cmoMduBase* parent ) : cmoMduBase(name, parent),
		nMisses( getStatU32BIT( "miss count" ) ), // estadistica 1
		anotherStat( getStatU32BIT( "Another count" ) ) // estadistica 2
	{		
	}

	// stupid clock
	void clock( U32 cycle ) 
	{ 		
		nMisses++; // se usan como una variable cualquiera...
		if ( cycle % 2 == 0 )
			anotherStat ++;
	}

	~Box1() 
	{
		cout << "Me destruyo ( soy '" << getName() << "' )" << endl;

	}
};


int main()
{

	Box1* b1 = new Box1("cmoMduBase b1",0);

	// boxes registered until now ( 1 )
	cmoMduBase::dumpNameBoxes();
	
	for ( U32 cycle = 0; cycle < 10; cycle++ ) {
		b1->clock(cycle);
	}
	
	delete b1;
	
	// get statistics...
	StatisticsManager::getManager().dump();


	

	return 0;
}