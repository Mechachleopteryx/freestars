/*
 * Copyright (C) 2014 Valery Kholodkov
 */
#include <cmath>

#include <QSignalMapper>

#include "ship_design_dialog.h"

namespace FreeStars {

struct CompCategory {
    const char *title;
    long mask;
};

static CompCategory starbaseCategories[] = {
    { "All", CT_ARMOR | CT_SHIELD | CT_WEAPON | CT_ELEC | CT_MECH | CT_ORBITAL },
    { "Armor", CT_ARMOR },
    { "Shields", CT_SHIELD },
    { "Weapons", CT_WEAPON },
    { "Electrical", CT_ELEC },
    { "Mechanical", CT_MECH },
    { "Orbital", CT_ORBITAL },
    { NULL, 0 }
};

static CompCategory shipCategories[] = {
    { "All", CT_ARMOR | CT_SHIELD | CT_WEAPON | CT_BOMB | CT_ELEC | CT_ENGINE | CT_MINELAY 
        | CT_MINER | CT_SCANNER | CT_MECH },
    { "Armor", CT_ARMOR },
    { "Shields", CT_SHIELD },
    { "Weapons", CT_WEAPON },
    { "Bombs", CT_BOMB },
    { "Electrical", CT_ELEC },
    { "Engines", CT_ENGINE },
    { "Mine layers", CT_MINELAY },
    { "Mining Robots", CT_MINER },
    { "Scanner", CT_SCANNER },
    { "Mechanical", CT_MECH },
    { NULL, 0 }
};

ShipDesignDialog::ShipDesignDialog(Player *_player, QWidget *parent)
    : QDialog(parent)
    , player(_player)
    , currentDesignMode(SDDDM_SHIPS)
    , currentViewMode(SDDVM_EXISTING)
{
    setupUi(this);

    QSignalMapper *designModeMapper = new QSignalMapper(this);

    designModeMapper->setMapping(shipDesignsButton, SDDDM_SHIPS);
    designModeMapper->setMapping(starbaseDesignsButton, SDDDM_STARBASES);

    connect(shipDesignsButton, SIGNAL(clicked(bool)), designModeMapper, SLOT(map()));
    connect(starbaseDesignsButton, SIGNAL(clicked(bool)), designModeMapper, SLOT(map()));

    QSignalMapper *viewModeMapper = new QSignalMapper(this);

    viewModeMapper->setMapping(existingDesignsButton, SDDVM_EXISTING);
    viewModeMapper->setMapping(availableHullTypesButton, SDDVM_AVAILABLE);
    viewModeMapper->setMapping(enemyHullsButton, SDDVM_ENEMY);
    viewModeMapper->setMapping(componentsButton, SDDVM_COMPONENTS);

    connect(existingDesignsButton, SIGNAL(clicked(bool)), viewModeMapper, SLOT(map()));
    connect(availableHullTypesButton, SIGNAL(clicked(bool)), viewModeMapper, SLOT(map()));
    connect(enemyHullsButton, SIGNAL(clicked(bool)), viewModeMapper, SLOT(map()));
    connect(componentsButton, SIGNAL(clicked(bool)), viewModeMapper, SLOT(map()));

    connect(designModeMapper, SIGNAL(mapped(int)), this, SLOT(setDesignMode(int)));
    connect(viewModeMapper, SIGNAL(mapped(int)), this, SLOT(setViewMode(int)));

    connect(copyDesignButton, SIGNAL(clicked(bool)), this, SLOT(copyDesign()));
    connect(editDesignButton, SIGNAL(clicked(bool)), this, SLOT(editDesign()));
    connect(deleteDesignButton, SIGNAL(clicked(bool)), this, SLOT(deleteDesign()));

    connect(doneButton, SIGNAL(clicked(bool)), this, SLOT(accept()));

    populateExistingDesigns(SDDDM_SHIPS);
}

void ShipDesignDialog::setDesignMode(int mode)
{
    switchMode(currentDesignMode, mode, currentViewMode, currentViewMode);
    currentDesignMode = mode;
}

void ShipDesignDialog::setViewMode(int mode)
{
    switchMode(currentDesignMode, currentDesignMode, currentViewMode, mode);
    currentViewMode = mode;
}

void ShipDesignDialog::switchMode(int oldDesignMode, int newDesignMode, int oldViewMode, int newViewMode)
{
    if (oldViewMode == SDDVM_COMPONENTS) {
        chooseComponentBox1->clear();
        chooseComponentBox1->disconnect();

        if (newViewMode != SDDVM_COMPONENTS) {
            copyDesignButton->setEnabled(true);
            editDesignButton->setEnabled(true);
            deleteDesignButton->setEnabled(true);
        }
    }
    else {
        chooseDesignBox->clear();

        if (oldViewMode == SDDVM_AVAILABLE || oldViewMode == SDDVM_ENEMY) {
            deleteDesignButton->setEnabled(true);
        }
    }

    stackedWidget2->setCurrentIndex(newViewMode == SDDVM_COMPONENTS ? 1 : 0);

    if (newViewMode == SDDVM_COMPONENTS) {
        struct CompCategory *categories = newDesignMode == SDDDM_SHIPS ? shipCategories : starbaseCategories;

        while (categories->title != NULL) {
            chooseComponentBox1->addItem(tr(categories->title), QVariant((qlonglong)categories->mask));
            categories++;
        }

        connect(chooseComponentBox1, SIGNAL(activated(int)), this, SLOT(setComponentCategory(int)));
        setComponentCategory(0);

        if (oldViewMode != SDDVM_COMPONENTS) {
            copyDesignButton->setEnabled(false);
            editDesignButton->setEnabled(false);
            deleteDesignButton->setEnabled(false);
        }
    }
    else {
        if (newViewMode == SDDVM_EXISTING) {
            populateExistingDesigns(newDesignMode);
        }
        else if (newViewMode == SDDVM_AVAILABLE) {
            populateAvailableHullTypes(newDesignMode);
        }

        if (newViewMode == SDDVM_AVAILABLE || newViewMode == SDDVM_ENEMY) {
            deleteDesignButton->setEnabled(false);
        }
    }
}

void ShipDesignDialog::setComponentCategory(int index)
{
    QVariant userData = chooseComponentBox1->itemData(index);

    long mask = userData.toULongLong();

    componentListWidget1->clear();

    const std::deque<Component*> &components = TheGame->GetComponents();

    for (std::deque<Component*>::const_iterator i = components.begin() ; i != components.end() ; i++) {
        if ((*i)->IsBuildable(player) && ((*i)->GetType() & mask)) {
            componentListWidget1->addItem(new QListWidgetItem(QString((*i)->GetName().c_str())));
        }
    }
}

void ShipDesignDialog::populateExistingDesigns(int designMode)
{
    int max = designMode == SDDDM_SHIPS ? Rules::GetConstant("MaxShipDesigns") 
        : Rules::GetConstant("MaxBaseDesigns");

    for (long i = 0 ; i != max ; i++) {
        const Ship *ship = designMode == SDDDM_SHIPS ? player->GetShipDesign(i) : player->GetBaseDesign(i);

        if (ship != NULL) {
            chooseDesignBox->addItem(QString(ship->GetName().c_str()));
        }
    }
}

void ShipDesignDialog::populateAvailableHullTypes(int designMode)
{
    const std::deque<Component*> &components = TheGame->GetComponents();

    for (std::deque<Component*>::const_iterator i = components.begin(); i != components.end(); i++) {
        if ((*i)->IsBuildable(player) && ((*i)->IsType(designMode == SDDDM_SHIPS ? CT_HULL : CT_BASE))) {
            chooseDesignBox->addItem(QString((*i)->GetName().c_str()));
        }
    }
}

void ShipDesignDialog::copyDesign()
{
}

void ShipDesignDialog::editDesign()
{
}

void ShipDesignDialog::deleteDesign()
{
}

};
