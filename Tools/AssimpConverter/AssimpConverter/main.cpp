#include "stdafx.h"
#include "AssimpConverter.h"

int main()
{
	AssimpConverter Converter{};

	Converter.LoadFromFiles("../Models/Sporty Granny.fbx");
	//Converter.ShowMeshData();
	Converter.Serialize("../Exported", "Granny");


}