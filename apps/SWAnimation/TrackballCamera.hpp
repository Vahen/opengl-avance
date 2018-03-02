#ifndef __TRACKBALL_CAMERA_H__
#define __TRACKBALL_CAMERA_H__

#include <glm/vec3.hpp>
#include "Camera.hpp"

class TrackballCamera : public Camera {

private:

	float m_fDistance, m_fAngleX, m_fAngleY;
	glm::vec3 m_target;

public:

	TrackballCamera();

	TrackballCamera(float Distance, float AngleX, float AngleY,glm::vec3 target);

	/**
	 * Permet d'avancer ou de reculer la camera
	 * @param t : valeur a enlever ou rajouter
	 */
	void moveFront(float delta);


	void rotateLeft(float degrees);

	/**
	* Permet de faire tourner la camera vers le haut ou le bas(en ajoutant ou en enlevant des degrées)
	* @param degrees : valeur en degrees a enlever ou ajouter
	*/
	void rotateUp(float degrees);

	/**
	 * Calcul la viewMatrix de la camera et la renvoie
	 * @return La viewMatrix de la camera
	 */
	glm::mat4 getViewMatrix() const override;

};

#endif

