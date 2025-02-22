#include "ofApp.h"
#include "ofUtils.h"

#include <iostream>
#include <vector>
#include <random>

// parameters for GUI
constexpr float xshift = 400;
constexpr float yshift = 80;
constexpr float anchor = 0;
constexpr float length = 70;
constexpr float p1x = anchor + xshift;
constexpr float p1y = anchor + yshift;
constexpr float p2x = anchor + length + xshift;
constexpr float p2y = anchor + yshift;
constexpr float p3x = anchor + length + xshift;
constexpr float p3y = anchor + length + yshift;
constexpr float p4x = anchor + xshift;
constexpr float p4y = anchor + length + yshift;
constexpr float rr = 8;

//int countThresh = 0;
std::string fps_text;
std::string physic_text;

//Simulation parameters
int cntFps = 0;
clock_t now, lastTime, delta;
clock_t physic_begin, physic_delta;

//Particle groups by color
std::vector<point> green;
std::vector<point> red;
std::vector<point> white;
std::vector<point> blue;

//Subdivison grid
grid subdiv;

/**
 * @brief Return a random float in range [a,b[
 *
 * @param a lower bound
 * @param b higher bound
 * @return random float
 */
inline float RandomFloat(const float a, const float b)
{
	const float diff = b - a;
	const float r = ofRandomuf() * diff;
	return a + r;
}

/**
 * @brief Draw all point from a given group of vector<point>*
 *
 * @param points a group of point
 */
void Draw(const std::vector<point>* points)
{
	for (auto& point : *points) point.draw();
}

/**
 * @brief Generate a number of single colored points randomly distributed on canvas
 *
 * @param num number of point to generate
 * @param r red
 * @param g green
 * @param b blue
 * @return a group of random point
 */
std::vector<point> CreatePoints(const int num, const int r, const int g, const int b)
{
	std::vector<point> points;
	points.reserve(num);
	for (auto i = 0; i < num; i++)
	{
		int x = static_cast<int>(ofRandomWidth());
		int y = static_cast<int>(ofRandomHeight());
		points.emplace_back(x, y, r, g, b);
	}
	return points;
}

/**
 * @brief Interaction between 2 particle groups
 * @param Group1 the group that will be modified by the interaction
 * @param Group2 the interacting group (its value won't be modified)
 * @param G gravity coefficient
 * @param radius radius of interaction
 */
void ofApp::interaction(std::vector<point>* Group1, const std::vector<point>* Group2, const float G, const float radius)
{
	const float g = G / -100;	//Gravity coefficient
	const auto group1size = Group1->size();
	const auto group2size = Group2->size();
	const bool radius_toggle = radiusToogle;

	boundHeight = ofGetHeight();
	boundWidth = ofGetWidth();

	#pragma omp parallel
	{
		std::random_device rd;
		#pragma omp for
		for (auto i = 0; i < group1size; i++)
		{
			if (rd() % 100 < probability) {
				auto& p1 = (*Group1)[i];
				float fx = 0;
				float fy = 0;

				//This inner loop is, of course, where most of the CPU time is spent. Everything else is cheap
				for (auto j = 0; j < group2size; j++)
				{
					const auto& p2 = (*Group2)[j];

					// you don't need sqrt to compare distance. (you need it to compute the actual distance however)
					const auto dx = p1.x - p2.x;
					const auto dy = p1.y - p2.y;
					const auto r = dx * dx + dy * dy;

					//Calculate the force in given bounds. 
					if ((r < radius * radius || radius_toggle) && r != 0.0F)
					{
						fx += (dx / std::sqrt(dx * dx + dy * dy));
						fy += (dy / std::sqrt(dx * dx + dy * dy));
					}
				}

				//Calculate new velocity
				p1.vx = (p1.vx + (fx * g)) * (1.0 - viscosity);
				p1.vy = (p1.vy + (fy * g)) * (1.0 - viscosity) + worldGravity;

				// Wall Repel
				if (wallRepel > 0.0F)
				{
					if (p1.x < wallRepel) p1.vx += (wallRepel - p1.x) * 0.1;
					if (p1.y < wallRepel) p1.vy += (wallRepel - p1.y) * 0.1;
					if (p1.x > boundWidth - wallRepel) p1.vx += (boundWidth - wallRepel - p1.x) * 0.1;
					if (p1.y > boundHeight - wallRepel) p1.vy += (boundHeight - wallRepel - p1.y) * 0.1;
				}

				//Update position based on velocity
				p1.x += p1.vx;
				p1.y += p1.vy;

				//Checking for canvas bounds
				if (boundsToggle)
				{
					if (p1.x < 0)
					{
						p1.vx *= -1;
						p1.x = 0;
					}
					if (p1.x > boundWidth)
					{
						p1.vx *= -1;
						p1.x = boundWidth;
					}
					if (p1.y < 0)
					{
						p1.vy *= -1;
						p1.y = 0;
					}
					if (p1.y > boundHeight)
					{
						p1.vy *= -1;
						p1.y = boundHeight;
					}
				}
			}
		}
	}
}

/* omp end parallel */

/**
 * @brief Generate new sets of points
 */
void ofApp::restart()
{
	if (numberSliderG > 0) { green = CreatePoints(numberSliderG, 100, 250, 10); }
	if (numberSliderR > 0) { red = CreatePoints(numberSliderR, 250, 10, 100); }
	if (numberSliderW > 0) { white = CreatePoints(numberSliderW, 250, 250, 250); }
	if (numberSliderB > 0) { blue = CreatePoints(numberSliderB, 100, 100, 250); }
}


/**
 * @brief Generate initial simulation parameters
 */
void ofApp::random()
{
	// GREEN
	//numberSliderG = RandomFloat(0, 3000);
	powerSliderGG = RandomFloat(-100, 100) * forceVariance;
	powerSliderGR = RandomFloat(-100, 100) * forceVariance;
	powerSliderGW = RandomFloat(-100, 100) * forceVariance;
	powerSliderGB = RandomFloat(-100, 100) * forceVariance;

	vSliderGG = RandomFloat(10, 200) * radiusVariance;
	vSliderGR = RandomFloat(10, 200) * radiusVariance;
	vSliderGW = RandomFloat(10, 200) * radiusVariance;
	vSliderGB = RandomFloat(10, 200) * radiusVariance;

	// RED
	//numberSliderR = RandomFloat(0, 3000);
	powerSliderRR = RandomFloat(-100, 100) * forceVariance;
	powerSliderRG = RandomFloat(-100, 100) * forceVariance;
	powerSliderRW = RandomFloat(-100, 100) * forceVariance;
	powerSliderRB = RandomFloat(-100, 100) * forceVariance;

	vSliderRG = RandomFloat(10, 200) * radiusVariance;
	vSliderRR = RandomFloat(10, 200) * radiusVariance;
	vSliderRW = RandomFloat(10, 200) * radiusVariance;
	vSliderRB = RandomFloat(10, 200) * radiusVariance;

	// WHITE
	// numberSliderW = RandomFloat(0, 3000);
	powerSliderWW = RandomFloat(-100, 100) * forceVariance;
	powerSliderWR = RandomFloat(-100, 100) * forceVariance;
	powerSliderWG = RandomFloat(-100, 100) * forceVariance;
	powerSliderWB = RandomFloat(-100, 100) * forceVariance;

	vSliderWG = RandomFloat(10, 200) * radiusVariance;
	vSliderWR = RandomFloat(10, 200) * radiusVariance;
	vSliderWW = RandomFloat(10, 200) * radiusVariance;
	vSliderWB = RandomFloat(10, 200) * radiusVariance;

	// BLUE
	//numberSliderB = RandomFloat(0, 3000);
	powerSliderBB = RandomFloat(-100, 100) * forceVariance;
	powerSliderBW = RandomFloat(-100, 100) * forceVariance;
	powerSliderBR = RandomFloat(-100, 100) * forceVariance;
	powerSliderBG = RandomFloat(-100, 100) * forceVariance;

	vSliderBG = RandomFloat(10, 200) * radiusVariance;
	vSliderBR = RandomFloat(10, 200) * radiusVariance;
	vSliderBW = RandomFloat(10, 200) * radiusVariance;
	vSliderBB = RandomFloat(10, 200) * radiusVariance;
}

/// this is a cheap and quick way to save and load parameters (openFramework have betters ways but requires some additional library setups) 
// Dialog gui tested on windows machine only. Not sure if it works on Mac or Linux too.
void ofApp::saveSettings()
{
	const std::vector<float> settings = {
		powerSliderGG, powerSliderGR, powerSliderGW, powerSliderGB,
		vSliderGG, vSliderGR, vSliderGW, vSliderGB,
		powerSliderRG, powerSliderRR, powerSliderRW, powerSliderRB,
		vSliderRG, vSliderRR, vSliderRW, vSliderRB,
		powerSliderWG, powerSliderWR, powerSliderWW, powerSliderWB,
		vSliderWG, vSliderWR, vSliderWW, vSliderWB,
		powerSliderBG, powerSliderBR, powerSliderBW, powerSliderBB,
		vSliderBG, vSliderBR, vSliderBW, vSliderBB,
		static_cast<float>(numberSliderG),
		static_cast<float>(numberSliderR),
		static_cast<float>(numberSliderW),
		static_cast<float>(numberSliderB),
		viscoSlider
	};

	std::string save_path;
	ofFileDialogResult result = ofSystemSaveDialog("model.txt", "Save");
	if (result.bSuccess)
	{
		save_path = result.getPath();
	}
	else
	{
		ofSystemAlertDialog("Could not Save Model!");
	}
	ofstream myfile(save_path);
	if (myfile.is_open())
	{
		for (int i = 0; i < settings.size(); i++)
		{
			myfile << settings[i] << " ";
		}
		myfile.close();
		std::cout << "file saved successfully";
	}
	else
	{
		std::cout << "unable to save file!";
	}
}

// Dialog gui tested on windows machine only. Not sure if it works on Mac or Linux too.
void ofApp::loadSettings()
{
	std::string load_path;
	std::string text;
	ofFileDialogResult result = ofSystemLoadDialog("Load file", false, load_path);
	if (result.bSuccess)
	{
		load_path = result.getPath();
		std::ifstream t(load_path);
		std::stringstream loaded;
		loaded << t.rdbuf();
		text = loaded.str();
	}
	else
	{
		ofSystemAlertDialog("Could not Load the File!");
	}

	// split text by space and convert them to floats
	string word;
	std::vector<float> p;
	for (auto x : text)
	{
		if (x == ' ')
		{
			p.push_back(std::stof(word));
			word = "";
		}
		else
		{
			word += x;
		}
	}

	if (p.size() < 37)
	{
		// better checks needed
		ofSystemAlertDialog("Could not read the file!");
	}
	else
	{
		powerSliderGG = p[0];
		powerSliderGR = p[1];
		powerSliderGW = p[2];
		powerSliderGB = p[3];
		vSliderGG = p[4];
		vSliderGR = p[5];
		vSliderGW = p[6];
		vSliderGB = p[7];
		powerSliderRG = p[8];
		powerSliderRR = p[9];
		powerSliderRW = p[10];
		powerSliderRB = p[11];
		vSliderRG = p[12];
		vSliderRR = p[13];
		vSliderRW = p[14];
		vSliderRB = p[15];
		powerSliderWG = p[16];
		powerSliderWR = p[17];
		powerSliderWW = p[18];
		powerSliderWB = p[19];
		vSliderWG = p[20];
		vSliderWR = p[21];
		vSliderWW = p[22];
		vSliderWB = p[23];
		powerSliderBG = p[24];
		powerSliderBR = p[25];
		powerSliderBW = p[26];
		powerSliderBB = p[27];
		vSliderBG = p[28];
		vSliderBR = p[29];
		vSliderBW = p[30];
		vSliderBB = p[31];
		numberSliderG = static_cast<int>(p[32]);
		numberSliderR = static_cast<int>(p[33]);
		numberSliderW = static_cast<int>(p[34]);
		numberSliderB = static_cast<int>(p[35]);
		viscoSlider = p[36];
	}
	probabilitySlider = 100;
	restart();
}


//------------------------------GUI initialization------------------------------
void ofApp::setup()
{
	lastTime = clock();
	ofSetWindowTitle("Particle Life - www.brainxyz.com");
	ofSetVerticalSync(false);

	// Interface
	gui.setup("Settings");
	gui.loadFont("Arial", 12);
	gui.setWidthElements(300.0f);

	gui.add(fps.setup("FPS", "0"));

	gui.add(resetButton.setup("Restart"));
	gui.add(randomChoice.setup("Randomize (space bar)"));
	gui.add(save.setup("Save Model"));
	gui.add(load.setup("Load Model"));
	gui.add(modelToggle.setup("Show Model", false));

	globalGroup.setup("Global");
	globalGroup.add(viscoSlider.setup("Viscosity/Friction", viscosity, 0, 1));
	globalGroup.add(gravitySlider.setup("Gravity", worldGravity, -1, 1));
	globalGroup.add(wallRepelSlider.setup("Wall Repel", wallRepel, 0, 100));
	globalGroup.add(boundsToggle.setup("Bounded", false));

	gui.add(&globalGroup);

	// Quantity
	qtyGroup.setup("Quantity (require restart/randomize)");
	qtyGroup.add(numberSliderG.setup("Green", pnumberSliderG, 0, 10000));
	qtyGroup.add(numberSliderR.setup("Red", pnumberSliderR, 0, 10000));
	qtyGroup.add(numberSliderW.setup("White", pnumberSliderW, 0, 10000));
	qtyGroup.add(numberSliderB.setup("Blue", pnumberSliderB, 0, 10000));
	gui.add(&qtyGroup);

	// GREEN
	greenGroup.setup("Green");
	greenGroup.add(powerSliderGG.setup("green x green:", ppowerSliderGG, -100, 100));
	greenGroup.add(powerSliderGR.setup("green x red:", ppowerSliderGR, -100, 100));
	greenGroup.add(powerSliderGW.setup("green x white:", ppowerSliderGW, -100, 100));
	greenGroup.add(powerSliderGB.setup("green x blue:", ppowerSliderGB, -100, 100));

	greenGroup.add(vSliderGG.setup("radius g x g:", pvSliderGG, 10, 500));
	greenGroup.add(vSliderGR.setup("radius g x r:", pvSliderGR, 10, 500));
	greenGroup.add(vSliderGW.setup("radius g x w:", pvSliderGW, 10, 500));
	greenGroup.add(vSliderGB.setup("radius g x b:", pvSliderGB, 10, 500));

	greenGroup.minimize();
	gui.add(&greenGroup);

	// RED
	redGroup.setup("Red");
	redGroup.add(powerSliderRR.setup("red x red:", ppowerSliderRR, -100, 100));
	redGroup.add(powerSliderRG.setup("red x green:", ppowerSliderRG, -100, 100));
	redGroup.add(powerSliderRW.setup("red x white:", ppowerSliderRW, -100, 100));
	redGroup.add(powerSliderRB.setup("red x blue:", ppowerSliderRB, -100, 100));

	redGroup.add(vSliderRG.setup("radius r x g:", pvSliderRG, 10, 500));
	redGroup.add(vSliderRR.setup("radius r x r:", pvSliderRR, 10, 500));
	redGroup.add(vSliderRW.setup("radius r x w:", pvSliderRW, 10, 500));
	redGroup.add(vSliderRB.setup("radius r x b:", pvSliderRB, 10, 500));

	redGroup.minimize();
	gui.add(&redGroup);

	// WHITE
	whiteGroup.setup("White");
	whiteGroup.add(powerSliderWW.setup("white x white:", ppowerSliderWW, -100, 100));
	whiteGroup.add(powerSliderWR.setup("white x red:", ppowerSliderWR, -100, 100));
	whiteGroup.add(powerSliderWG.setup("white x green:", ppowerSliderWG, -100, 100));
	whiteGroup.add(powerSliderWB.setup("white x blue:", ppowerSliderWB, -100, 100));

	whiteGroup.add(vSliderWG.setup("radius w x g:", pvSliderWG, 10, 500));
	whiteGroup.add(vSliderWR.setup("radius w x r:", pvSliderWR, 10, 500));
	whiteGroup.add(vSliderWW.setup("radius w x w:", pvSliderWW, 10, 500));
	whiteGroup.add(vSliderWB.setup("radius w x b:", pvSliderWB, 10, 500));

	whiteGroup.minimize();
	gui.add(&whiteGroup);

	// BLUE
	blueGroup.setup("Blue");
	blueGroup.add(powerSliderBB.setup("blue x blue:", ppowerSliderBB, -100, 100));
	blueGroup.add(powerSliderBW.setup("blue x white:", ppowerSliderBW, -100, 100));
	blueGroup.add(powerSliderBR.setup("blue x red:", ppowerSliderBR, -100, 100));
	blueGroup.add(powerSliderBG.setup("blue x green:", ppowerSliderBG, -100, 100));

	blueGroup.add(vSliderBG.setup("radius b x g:", pvSliderBG, 10, 500));
	blueGroup.add(vSliderBR.setup("radius b x r:", pvSliderBR, 10, 500));
	blueGroup.add(vSliderBW.setup("radius b x w:", pvSliderBW, 10, 500));
	blueGroup.add(vSliderBB.setup("radius b x b:", pvSliderBB, 10, 500));

	blueGroup.minimize();
	gui.add(&blueGroup);

	expGroup.setup("Experimental");
	expGroup.add(evoToggle.setup("Evolve parameters", false));
	expGroup.add(evoProbSlider.setup("evo chance%", evoChance, 0, 100));
	expGroup.add(evoAmountSlider.setup("evo amount%%", evoAmount, 0, 100));
	expGroup.add(radiusToogle.setup("infinite radius", false));
	expGroup.add(probabilitySlider.setup("interaction prob%", probability, 1, 100));
	expGroup.add(motionBlurToggle.setup("Motion Blur", false));
	expGroup.add(physicLabel.setup("physic (ms)", "0"));

	expGroup.minimize();
	gui.add(&expGroup);

	ofSetBackgroundAuto(false);
	ofEnableAlphaBlending();

	restart();
}

//------------------------------Update simulation with sliders values------------------------------
void ofApp::update()
{
	physic_begin = clock();
	probability = probabilitySlider;
	viscosity = viscoSlider;
	worldGravity = gravitySlider;
	wallRepel = wallRepelSlider;
	evoChance = evoProbSlider;
	evoAmount = evoAmountSlider;

	if(evoToggle && ofRandom(1.0F) < (evoChance / 100.0F))
	{
		powerSliderRR = powerSliderRR + ((ofRandom(2.0F) - 1.0F) * (powerSliderRR.getMax() - powerSliderRR.getMin()) * (evoAmount / 100.0F));
		powerSliderRG = powerSliderRG + ((ofRandom(2.0F) - 1.0F) * (powerSliderRG.getMax() - powerSliderRG.getMin()) * (evoAmount / 100.0F));
		powerSliderRB = powerSliderRB + ((ofRandom(2.0F) - 1.0F) * (powerSliderRB.getMax() - powerSliderRB.getMin()) * (evoAmount / 100.0F));
		powerSliderRW = powerSliderRW + ((ofRandom(2.0F) - 1.0F) * (powerSliderRW.getMax() - powerSliderRW.getMin()) * (evoAmount / 100.0F));
		powerSliderGR = powerSliderGR + ((ofRandom(2.0F) - 1.0F) * (powerSliderGR.getMax() - powerSliderGR.getMin()) * (evoAmount / 100.0F));
		powerSliderGG = powerSliderGG + ((ofRandom(2.0F) - 1.0F) * (powerSliderGG.getMax() - powerSliderGG.getMin()) * (evoAmount / 100.0F));
		powerSliderGB = powerSliderGB + ((ofRandom(2.0F) - 1.0F) * (powerSliderGB.getMax() - powerSliderGB.getMin()) * (evoAmount / 100.0F));
		powerSliderGW = powerSliderGW + ((ofRandom(2.0F) - 1.0F) * (powerSliderGW.getMax() - powerSliderGW.getMin()) * (evoAmount / 100.0F));
		powerSliderBR = powerSliderBR + ((ofRandom(2.0F) - 1.0F) * (powerSliderBR.getMax() - powerSliderBR.getMin()) * (evoAmount / 100.0F));
		powerSliderBG = powerSliderBG + ((ofRandom(2.0F) - 1.0F) * (powerSliderBG.getMax() - powerSliderBG.getMin()) * (evoAmount / 100.0F));
		powerSliderBB = powerSliderBB + ((ofRandom(2.0F) - 1.0F) * (powerSliderBB.getMax() - powerSliderBB.getMin()) * (evoAmount / 100.0F));
		powerSliderBW = powerSliderBW + ((ofRandom(2.0F) - 1.0F) * (powerSliderBW.getMax() - powerSliderBW.getMin()) * (evoAmount / 100.0F));
		powerSliderWR = powerSliderWR + ((ofRandom(2.0F) - 1.0F) * (powerSliderWR.getMax() - powerSliderWR.getMin()) * (evoAmount / 100.0F));
		powerSliderWG = powerSliderWG + ((ofRandom(2.0F) - 1.0F) * (powerSliderWG.getMax() - powerSliderWG.getMin()) * (evoAmount / 100.0F));
		powerSliderWB = powerSliderWB + ((ofRandom(2.0F) - 1.0F) * (powerSliderWB.getMax() - powerSliderWB.getMin()) * (evoAmount / 100.0F));
		powerSliderWW = powerSliderWW + ((ofRandom(2.0F) - 1.0F) * (powerSliderWW.getMax() - powerSliderWW.getMin()) * (evoAmount / 100.0F));

		vSliderRR = vSliderRR + ((ofRandom(2.0F) - 1.0F) * (vSliderRR.getMax() - vSliderRR.getMin()) * (evoAmount / 100.0F));
		vSliderRG = vSliderRG + ((ofRandom(2.0F) - 1.0F) * (vSliderRG.getMax() - vSliderRG.getMin()) * (evoAmount / 100.0F));
		vSliderRB = vSliderRB + ((ofRandom(2.0F) - 1.0F) * (vSliderRB.getMax() - vSliderRB.getMin()) * (evoAmount / 100.0F));
		vSliderRW = vSliderRW + ((ofRandom(2.0F) - 1.0F) * (vSliderRW.getMax() - vSliderRW.getMin()) * (evoAmount / 100.0F));
		vSliderGR = vSliderGR + ((ofRandom(2.0F) - 1.0F) * (vSliderGR.getMax() - vSliderGR.getMin()) * (evoAmount / 100.0F));
		vSliderGG = vSliderGG + ((ofRandom(2.0F) - 1.0F) * (vSliderGG.getMax() - vSliderGG.getMin()) * (evoAmount / 100.0F));
		vSliderGB = vSliderGB + ((ofRandom(2.0F) - 1.0F) * (vSliderGB.getMax() - vSliderGB.getMin()) * (evoAmount / 100.0F));
		vSliderGW = vSliderGW + ((ofRandom(2.0F) - 1.0F) * (vSliderGW.getMax() - vSliderGW.getMin()) * (evoAmount / 100.0F));
		vSliderBR = vSliderBR + ((ofRandom(2.0F) - 1.0F) * (vSliderBR.getMax() - vSliderBR.getMin()) * (evoAmount / 100.0F));
		vSliderBG = vSliderBG + ((ofRandom(2.0F) - 1.0F) * (vSliderBG.getMax() - vSliderBG.getMin()) * (evoAmount / 100.0F));
		vSliderBB = vSliderBB + ((ofRandom(2.0F) - 1.0F) * (vSliderBB.getMax() - vSliderBB.getMin()) * (evoAmount / 100.0F));
		vSliderBW = vSliderBW + ((ofRandom(2.0F) - 1.0F) * (vSliderBW.getMax() - vSliderBW.getMin()) * (evoAmount / 100.0F));
		vSliderWR = vSliderWR + ((ofRandom(2.0F) - 1.0F) * (vSliderWR.getMax() - vSliderWR.getMin()) * (evoAmount / 100.0F));
		vSliderWG = vSliderWG + ((ofRandom(2.0F) - 1.0F) * (vSliderWG.getMax() - vSliderWG.getMin()) * (evoAmount / 100.0F));
		vSliderWB = vSliderWB + ((ofRandom(2.0F) - 1.0F) * (vSliderWB.getMax() - vSliderWB.getMin()) * (evoAmount / 100.0F));
		vSliderWW = vSliderWW + ((ofRandom(2.0F) - 1.0F) * (vSliderWW.getMax() - vSliderWW.getMin()) * (evoAmount / 100.0F));

		if(vSliderRR < 0.0F) vSliderRR = 0;
		if(vSliderRG < 0.0F) vSliderRG = 0;
		if(vSliderRB < 0.0F) vSliderRB = 0;
		if(vSliderRW < 0.0F) vSliderRW = 0;
		if(vSliderGR < 0.0F) vSliderGR = 0;
		if(vSliderGG < 0.0F) vSliderGG = 0;
		if(vSliderGB < 0.0F) vSliderGB = 0;
		if(vSliderGW < 0.0F) vSliderGW = 0;
		if(vSliderBR < 0.0F) vSliderBR = 0;
		if(vSliderBG < 0.0F) vSliderBG = 0;
		if(vSliderBB < 0.0F) vSliderBB = 0;
		if(vSliderBW < 0.0F) vSliderBW = 0;
		if(vSliderWR < 0.0F) vSliderWR = 0;
		if(vSliderWG < 0.0F) vSliderWG = 0;
		if(vSliderWB < 0.0F) vSliderWB = 0;
		if(vSliderWW < 0.0F) vSliderWW = 0;

		if (vSliderRR > 200.0F) vSliderRR = 200.0F;
		if (vSliderRG > 200.0F) vSliderRG = 200.0F;
		if (vSliderRB > 200.0F) vSliderRB = 200.0F;
		if (vSliderRW > 200.0F) vSliderRW = 200.0F;
		if (vSliderGR > 200.0F) vSliderGR = 200.0F;
		if (vSliderGG > 200.0F) vSliderGG = 200.0F;
		if (vSliderGB > 200.0F) vSliderGB = 200.0F;
		if (vSliderGW > 200.0F) vSliderGW = 200.0F;
		if (vSliderBR > 200.0F) vSliderBR = 200.0F;
		if (vSliderBG > 200.0F) vSliderBG = 200.0F;
		if (vSliderBB > 200.0F) vSliderBB = 200.0F;
		if (vSliderBW > 200.0F) vSliderBW = 200.0F;
		if (vSliderWR > 200.0F) vSliderWR = 200.0F;
		if (vSliderWG > 200.0F) vSliderWG = 200.0F;
		if (vSliderWB > 200.0F) vSliderWB = 200.0F;
		if (vSliderWW > 200.0F) vSliderWW = 200.0F;
	}

	if (numberSliderR > 0)
	{
		interaction(&red, &red, powerSliderRR, vSliderRR);
		if (numberSliderG > 0) interaction(&red, &green, powerSliderRG, vSliderRG);
		if (numberSliderB > 0) interaction(&red, &blue, powerSliderRB, vSliderRB);
		if (numberSliderW > 0) interaction(&red, &white, powerSliderRW, vSliderRW);
	}

	if (numberSliderG > 0)
	{
		if (numberSliderR > 0) interaction(&green, &red, powerSliderGR, vSliderGR);
		interaction(&green, &green, powerSliderGG, vSliderGG);
		if (numberSliderB > 0) interaction(&green, &blue, powerSliderGB, vSliderGB);
		if (numberSliderW > 0) interaction(&green, &white, powerSliderGW, vSliderGW);
	}

	if (numberSliderB > 0)
	{
		if (numberSliderR > 0) interaction(&blue, &red, powerSliderBR, vSliderBR);
		if (numberSliderG > 0) interaction(&blue, &green, powerSliderBG, vSliderBG);
		interaction(&blue, &blue, powerSliderBB, vSliderBB);
		if (numberSliderW > 0) interaction(&blue, &white, powerSliderBW, vSliderBW);
	}

	if (numberSliderW > 0)
	{
		if (numberSliderR > 0) interaction(&white, &red, powerSliderWR, vSliderWR);
		if (numberSliderG > 0) interaction(&white, &green, powerSliderWG, vSliderWG);
		if (numberSliderB > 0) interaction(&white, &blue, powerSliderWB, vSliderWB);
		interaction(&white, &white, powerSliderWW, vSliderWW);
	}

	if (save) { saveSettings(); }
	if (load) { loadSettings(); }
	physic_delta = clock() - physic_begin;
}

//--------------------------------------------------------------
void ofApp::draw()
{
	if (motionBlurToggle)
	{
		ofSetColor(0, 0, 0, 64);
		ofDrawRectangle(0, 0, boundWidth, boundHeight);
	}
	else
	{
		ofClear(0);
	}

	//fps counter
	cntFps++;
	now = clock();
	delta = now - lastTime;

	//Time step
	if (delta >= 1000)
	{
		lastTime = now;
		fps.setup("FPS", to_string(static_cast<int>((1000 / static_cast<float>(delta)) * cntFps)));
		physicLabel.setup("physics (ms)", to_string(physic_delta) );

		cntFps = 0;
	}

	//Check for GUI interaction
	if (resetButton) { restart(); }
	if (randomChoice)
	{
		random();
		restart();
	}
	if (numberSliderW > 0) { Draw(&white); }
	if (numberSliderR > 0) { Draw(&red); }
	if (numberSliderG > 0) { Draw(&green); }
	if (numberSliderB > 0) { Draw(&blue); }

	//Draw GUI
	if (modelToggle == true)
	{
		ofSetColor(0, 0, 0);
		ofDrawCircle(xshift, yshift, 150);

		ofSetLineWidth(5);
		ofSetColor(150.0F - powerSliderGR, 150.0F + powerSliderGR, 150);
		ofDrawLine(p1x, p1y - 10, p2x, p2y - 10);
		ofSetColor(150.0F - powerSliderRG, 150.0F + powerSliderRG, 150);
		ofDrawLine(p1x, p1y + 10, p2x, p2y + 10);
		ofSetColor(150.0F - powerSliderGW, 150.0F + powerSliderGW, 150);
		ofDrawLine(p3x, p3y - 10, p1x, p1y - 10);
		ofSetColor(150.0F - powerSliderWG, 150.0F + powerSliderWG, 150);
		ofDrawLine(p3x, p3y + 10, p1x, p1y + 10);

		ofSetColor(150.0F - powerSliderGB, 150.0F + powerSliderGB, 150);
		ofDrawLine(p4x - 10, p4y, p1x - 10, p1y);
		ofSetColor(150.0F - powerSliderBG, 150.0F + powerSliderBG, 150);
		ofDrawLine(p4x + 10, p4y, p1x + 10, p1y);

		ofSetColor(150.0F - powerSliderRW, 150.0F + powerSliderRW, 150);
		ofDrawLine(p2x - 10, p2y, p3x - 10, p3y);
		ofSetColor(150.0F - powerSliderWR, 150.0F + powerSliderWR, 150);
		ofDrawLine(p2x + 10, p2y, p3x + 10, p3y);

		ofSetColor(150.0F - powerSliderRB, 150.0F + powerSliderRB, 150);
		ofDrawLine(p2x, p2y - 10, p4x, p4y - 10);
		ofSetColor(150.0F - powerSliderBR, 150.0F + powerSliderBR, 150);
		ofDrawLine(p2x, p2y + 10, p4x, p4y + 10);

		ofSetColor(150.0F - powerSliderWB, 150.0F + powerSliderWB, 150);
		ofDrawLine(p3x, p3y - 10, p4x, p4y - 10);
		ofSetColor(150.0F - powerSliderBW, 150.0F + powerSliderBW, 150);
		ofDrawLine(p3x, p3y + 10, p4x, p4y + 10);

		ofNoFill();
		ofSetColor(150.0F - powerSliderGG, 150.0F + powerSliderGG, 150);
		ofDrawCircle(p1x - 20, p1y - 20, rr + 20);
		ofSetColor(150.0F - powerSliderRR, 150.0F + powerSliderRR, 150);
		ofDrawCircle(p2x + 20, p2y - 20, rr + 20);
		ofSetColor(150.0F - powerSliderWW, 150.0F + powerSliderWW, 150);
		ofDrawCircle(p3x + 20, p3y + 20, rr + 20);
		ofSetColor(150.0F - powerSliderBB, 150.0F + powerSliderBB, 150);
		ofDrawCircle(p4x - 20, p4y + 20, rr + 20);

		ofFill();
		ofSetColor(100, 250, 10);
		ofDrawCircle(p1x, p1y, rr);
		ofSetColor(250, 10, 100);
		ofDrawCircle(p2x, p2y, rr);
		ofSetColor(250, 250, 250);
		ofDrawCircle(p3x, p3y, rr);
		ofSetColor(100, 100, 250);
		ofDrawCircle(p4x, p4y, rr);
	}
	gui.draw();
}

void ofApp::keyPressed(int key)
{
	if(key == ' ')
	{
		random();
		restart();
	}
}
