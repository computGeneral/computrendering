#include "GPUMath.h"
#include <ostream.h>

void print_vector( const F32* qf );
void print_replicate( const F32 value );

int main() {



	F32 v1[] = { 0.0, 0.0, 1.0, 5.0 };
	F32 v2[] = { 2.0, 3.0, 2.0, 9.0 };
	

	cout << "CG1_ISA_OPCODE_DP3 de : ";
	print_vector( v1 );
	cout << " con ";
	print_vector( v2 );
	cout << endl;
	
	F32 vOut[4];

	GPUMath::CG1_ISA_OPCODE_DP4( v1, v2, vOut );
	//F32 result = GPUMath::CG1_ISA_OPCODE_DP3( v1[0], v1[1], v1[2], v2[0], v2[1], v2[2] );

	cout << "El resultado es: ";
	//print_replicate( result );
	print_vector( vOut );
	cout << endl;


	return 0;
}


void print_vector( const F32* qf ) {

	cout << "(" << qf[0] << "," << qf[1] << "," <<
		qf[2] << "," << qf[3] << ")";
}

void print_replicate( const F32 value ) {
	cout << "(" << value << "," << value << "," <<
		value << "," << value << ")";
	
}