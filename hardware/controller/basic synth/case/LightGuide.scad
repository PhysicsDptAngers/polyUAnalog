LED_x = 5.1;
LED_y = 5.5;
LED_z = 1.8;


x_carre_jour = 3.9; // avant

d_pcb_panel = 7;

module lightGuide()
{
translate([0,0,1.2])
{
union()
{

difference()
{
    // Pourtour
    cube([LED_x*1.15, LED_y*1.15, d_pcb_panel/2.0], center=true);
    translate([0,0,-LED_z/2])
    cube([LED_x, LED_y, LED_z], center=true);
}

translate([0,0,d_pcb_panel/2])
cube([x_carre_jour, x_carre_jour, d_pcb_panel/2 + 3], center=true);

}

}
}

// Définir les dimensions de l'objet à quadriller
largeur_objet = 10;
hauteur_objet = 10;

// Définir les dimensions du quadrillage
nombre_de_colonnes = 3;
nombre_de_lignes = 3;
espacement_entre_objets = 2;

// Créer le quadrillage en dupliquant l'objet
for (i = [0 : nombre_de_colonnes - 1]) {
    for (j = [0 : nombre_de_lignes - 1]) {
        translate([i * (largeur_objet + espacement_entre_objets), j * (hauteur_objet + espacement_entre_objets), 0])
            lightGuide();
    }
}