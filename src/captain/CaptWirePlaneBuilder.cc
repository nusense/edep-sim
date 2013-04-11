#include "CaptWirePlaneBuilder.hh"
#include "DSimBuilder.hh"

#include <DSimLog.hh>

#include <globals.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>

#include <G4Polyhedra.hh>
#include <G4Tubs.hh>

#include <cmath>

class CaptWirePlaneMessenger
    : public DSimBuilderMessenger {
private:
    CaptWirePlaneBuilder* fBuilder;
    G4UIcmdWithADoubleAndUnit* fApothemCMD;
    G4UIcmdWithADoubleAndUnit* fSpacingCMD;

public:
    CaptWirePlaneMessenger(CaptWirePlaneBuilder* c) 
        : DSimBuilderMessenger(c,"Control the drift region geometry."),
          fBuilder(c) {

        fApothemCMD
            = new G4UIcmdWithADoubleAndUnit(CommandName("length"),this);
        fApothemCMD->SetGuidance("Set the apothem of the drift region.");
        fApothemCMD->SetParameterName("apothem",false);
        fApothemCMD->SetUnitCategory("Length");

        fSpacingCMD = new G4UIcmdWithADoubleAndUnit(
            CommandName("wireSpacing"),this);
        fSpacingCMD->SetGuidance(
            "Set the wire spacing.");
        fSpacingCMD->SetParameterName("spacing",false);
        fSpacingCMD->SetUnitCategory("Length");
    }

    virtual ~CaptWirePlaneMessenger() {
        delete fApothemCMD;
        delete fSpacingCMD;
    }

    void SetNewValue(G4UIcommand *cmd, G4String val) {
        if (cmd==fApothemCMD) {
            fBuilder->SetApothem(fApothemCMD->GetNewDoubleValue(val));
        }
        else if (cmd==fSpacingCMD) {
            fBuilder->SetSpacing(fSpacingCMD->GetNewDoubleValue(val));
        }
        else {
            DSimBuilderMessenger::SetNewValue(cmd,val);
        }
    }

};

void CaptWirePlaneBuilder::Init(void) {
    SetMessenger(new CaptWirePlaneMessenger(this));
    SetApothem(1000*mm);
    SetSpacing(3*mm);
    SetHeight(1*mm);

    SetSensitiveDetector("drift","segment");

}

CaptWirePlaneBuilder::~CaptWirePlaneBuilder() {};

G4LogicalVolume *CaptWirePlaneBuilder::GetPiece(void) {

    double rInner[] = {0.0, 0.0};
    double rOuter[] = {GetApothem(), GetApothem()};
    double zPlane[] = {-GetHeight()/2, GetHeight()/2};

    G4LogicalVolume *logVolume
	= new G4LogicalVolume(new G4Polyhedra(GetName(),
                                              90*degree, 450*degree,
                                              6, 2,
                                              zPlane, rInner, rOuter),
                              FindMaterial("Argon_Liquid"),
                              GetName());

    if (GetSensitiveDetector()) {
        logVolume->SetSensitiveDetector(GetSensitiveDetector());
    }

    // Determine the maximum (possible) wire length.
    double maxLength = 2*GetApothem()/std::cos(30*degree);

    // Determine the number of wires.  There is always at least one wire.
    int wires = int (2.0*GetApothem()/GetSpacing());
    if (wires<1) wires = 1;

    /// \todo Does the current wire rotation make any sense?  We need to think
    /// about how the local wire coordinates should be defined (or if they
    /// even matter).
    G4RotationMatrix* wireRotation = new G4RotationMatrix(); 
    wireRotation->rotateX(90*degree);

    // The offset of the first wire.
    double baseOffset = - 0.5*GetSpacing()*(wires-1);
    for (int wire = 0; wire<wires; ++wire) {
        // The position of the wire.
        double wireOffset = baseOffset + wire*GetSpacing();
        double wireLength = maxLength-2*std::abs(wireOffset)*std::tan(30*deg);
        double wireDiameter = std::min(GetHeight(), 
                                       GetSpacing()/4);
        G4LogicalVolume* logWire 
            = new G4LogicalVolume(new G4Tubs(GetName()+"/Wire",
                                             0.0, wireDiameter,
                                             wireLength/2,
                                             0*degree, 360*degree),
                                  FindMaterial("Captain_Wire"),
                                  GetName()+"/Wire");
        new G4PVPlacement(wireRotation,             // rotation.
                          G4ThreeVector(wireOffset,0,0), // position
                          logWire,                  // logical volume
                          logWire->GetName(),       // name
                          logVolume,                // mother  volume
                          false,                    // (not used)
                          0,                        // Copy number (zero)
                          false);                 // Check overlaps.
    }

    if (GetVisible()) {
        // Draw this as if it was a wire plane.
        logVolume->SetVisAttributes(GetColor(FindMaterial("Captain_Wire")));
    } 
    else {
        logVolume->SetVisAttributes(G4VisAttributes::Invisible);
    }
    
    return logVolume;
}