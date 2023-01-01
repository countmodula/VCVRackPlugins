struct AddExpanderMenuItem : MenuItem {
	Module* module;
	Model* model;
	Vec position;
	bool expandLeft = false;
	bool immediateRight = false;
	std::string expanderName;
	
	void onAction(const event::Action &e) override {
		// create module 
		engine::Module* module = model->createModule();
		APP->engine->addModule(module);

		// create modulewidget
		ModuleWidget* mw = model->createModuleWidget(module);
		if (mw) {
			if (expandLeft) {
				setModulePosNearestLeft(mw, position);
			}
			else {
				if (immediateRight) {
					APP->scene->rack->setModulePosForce(mw, position);
				}
				else {
					setModulePosNearestRight(mw, position);
				}
			}
			APP->scene->rack->addModule(mw);
			history::ModuleAdd *h = new history::ModuleAdd;
			h->name = rack::string::f("add %s expander", expanderName.c_str());
			h->setModule(mw);
			APP->history->push(h);
		}
	}	
};