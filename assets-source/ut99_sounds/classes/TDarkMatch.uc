//=============================================================================
// TDarkMatch.
//=============================================================================
class TDarkMatch extends DeathMatchPlus;

function AddDefaultInventory(pawn aPlayer)
{
	local SearchLight s;

	Super.AddDefaultInventory(aPlayer);
	
	//spawn a searchlight
	if ( aPlayer.IsA('Spectator') 
		|| aPlayer.FindInventoryType(class'SearchLight') != None )
		return;
	class'SearchLight'.default.bDefaultInventory = true;
	s = Spawn(class'SearchLight',,, aPlayer.Location);
	class'SearchLight'.default.bDefaultInventory = false;
	if (s != None)
	{
		s.bHeldItem = true;
		s.GiveTo( aPlayer );
		s.Activate();
		aPlayer.SelectedItem = s;
	}
}

defaultproperties
{
      MapListType=Class'Botpack.TDKMapList'
      MapPrefix="TDK"
      GameName="Tournament DarkMatch"
}
