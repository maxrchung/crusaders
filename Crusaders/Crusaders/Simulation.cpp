#include "Beatmap.hpp"
#include "Camera.hpp"
#include "Sprite.hpp"
#include "ObjectLine.hpp"
#include "ObjectSprite.hpp"
#include "ObjectPoints.hpp"
#include "Path.hpp"
#include "Simulation.hpp"

Simulation::Simulation(Path* path) 
	: time(0), timeEnd(10000), delta(200), dps((float)delta / 1000), state(SimulationState::Level1), simulationRunning(true), path(path), markerDistance(1000.0f) {
	camera = new Camera(this);

	// Fill remainingMarkers
	for (auto marker : path->markers) {
		marker.setObjectPoints(this);
		remainingMarkers.push(marker);
	}
	fillCurrentMarkers();
}

void Simulation::Run() {
	while (simulationRunning) {
		std::cout << "Processing: " << time << std::endl;

		Update();
		Draw();
		DrawRender();

		time += delta;
		if (time > timeEnd) {
			simulationRunning = false;
		}
	}
}
void Simulation::Update() {
	Vector3 camPos(0, 100, -time);
	camera->MoveTo(camPos);
	fillCurrentMarkers();

	while (!currentMarkers.empty()) {
		if (currentMarkers.front().position.z > camPos.z) {
			currentMarkers.pop_front();
		}
		else {
			break;
		}
	}
}

void Simulation::DrawLoad(ObjectPoints* objectPoints) {
	loadObjectPoints.push_back(objectPoints);
}

void Simulation::Draw() {
}

void Simulation::DrawRender() {
	// http://stackoverflow.com/questions/21622956/how-to-convert-direction-vector-to-euler-angles
	// Be careful using the above. It's okay in explaining some of the theory,
	// but the axes are not the same.
	// The above is still a good reference, but having to take care of a lot of wanky cases
	// when angles are negative can be really difficult. I decided to make my life easy to track
	// both a direction Vector3 and also the actual rotation amounts with directionRotations.
	Vector3 camPos = camera->position;
	Vector3 camRot = camera->directionRotations;

	for (auto objectPoints : loadObjectPoints) {
		auto& objectLines = objectPoints->objectLines;
		auto& objectSprites = objectPoints->objectSprites;
		for (int i = 0; i < objectLines.size(); ++i) {
			ObjectSprite* objectSprite = objectSprites[i];
			Sprite* sprite = objectSprite->sprite;
			Vector3 startCamCon = camera->ConvertPoint(*objectLines[i]->start, camPos, camRot);
			Vector3 endCamCon = camera->ConvertPoint(*objectLines[i]->end, camPos, camRot);

			// If both points are behind camera, don't draw it
			if (startCamCon.z >= 0 || endCamCon.z >= 0) {
				sprite->Fade(time, time, 0.0f, 0.0f);
				objectSprite->reset = true;
				continue;
			}

			Vector2 startPoint = camera->ApplyPerspective(startCamCon, endCamCon);
			Vector2 endPoint = camera->ApplyPerspective(endCamCon, startCamCon);
			Vector2 diff = endPoint - startPoint;
			float additionalRot = Vector2(sprite->rotation).AngleBetween(diff);
			float dist = diff.Magnitude();
			if (objectSprite->reset) {
				sprite->Fade(time, time, 1.0f, 1.0f);
				sprite->Move(time, time + delta, startPoint, startPoint);
				sprite->Rotate(time, time + delta, sprite->rotation + additionalRot, sprite->rotation + additionalRot);
				sprite->ScaleVector(time, time + delta, Vector2(dist, 1), Vector2(dist, 1));
				objectSprite->reset = false;
			}
			else {
				sprite->Move(time, time + delta, sprite->position, startPoint);
				sprite->Rotate(time, time + delta, sprite->rotation, sprite->rotation + additionalRot);
				sprite->ScaleVector(time, time + delta, sprite->scaleVector, Vector2(dist, 1));
			}
		}
	}
	loadObjectPoints.clear();
}

void Simulation::fillCurrentMarkers() {
	while (!remainingMarkers.empty()) {
		Vector3 distance = remainingMarkers.front().position - camera->position;
		float magnitude = distance.Magnitude();

		if (magnitude <= markerDistance) {
			currentMarkers.push_back(remainingMarkers.front());
			remainingMarkers.pop();
		}
		else {
			break;
		}
	}
}