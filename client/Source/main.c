#include "config.h"
#include "hid.h"
#include "raylib.h"
#include "raymath.h"
#include "socketshit.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SWIDTH 619
#define SHEIGHT 360

config cfg;
Font buttonFont;
Font fatFont;
int global = 0;
Color buttonColor, stickColor, controllerColorLeft, controllerColorRight;
HidNpadButton gyroCalibrateButton = HidNpadButton_Y;
bool gyroCalibrateButtonHeld = false;

Model _controllerModel, _joyLeftModel, _joyRightModel;

int countDigits(int value) {
  if (value < 0)
    value = -value;

  int count = 0;
  do {
    count++;
    value /= 10;
  } while (value != 0);

  return count;
}

void drawStick(Vector2 pos, HidAnalogStickState stick, HidNpadButton button, int materialIndex,
                int materialIndexJoyL, int materialIndexJoyR) {
  Vector2 at;
  at.x = pos.x + stick.x / 800;
  at.y = pos.y + -stick.y / 800;
  DrawRing(pos, 56, 50, 0, 360, 40, buttonColor);
  DrawCircleV(at, 40,
              getPacketData()->keys & button ? *(Color *)&cfg.colActive
                                             : stickColor);
  /* debugging code
  char buf[64];
  snprintf(buf, sizeof(buf), "%d", global);
  at.x -= countDigits(global) * 3;
  at.y -= 7;
  DrawTextEx(fatFont, buf, at, 9, 2, WHITE);
  */
  if (materialIndex != -1) {
    _controllerModel.materials[materialIndex].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : buttonColor;
  }
  if (materialIndexJoyL != -1) {
    _joyLeftModel.materials[materialIndexJoyL].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : buttonColor;
  }
  if (materialIndexJoyR != -1) {
    _joyRightModel.materials[materialIndexJoyR].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : buttonColor;
  }
}

Quaternion toQuaternion(float m[3][3]) {
  Quaternion q;

  float trace = m[0][0] + m[1][1] + m[2][2];
  if (trace > 0) {
    float s = 0.5f / sqrtf(trace + 1.0f);
    q.w = 0.25f / s;
    q.x = (m[2][1] - m[1][2]) * s;
    q.y = (m[0][2] - m[2][0]) * s;
    q.z = (m[1][0] - m[0][1]) * s;
  } else {
    if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
      float s = 2.0f * sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]);
      q.w = (m[2][1] - m[1][2]) / s;
      q.x = 0.25f * s;
      q.y = (m[0][1] + m[1][0]) / s;
      q.z = (m[0][2] + m[2][0]) / s;
    } else if (m[1][1] > m[2][2]) {
      float s = 2.0f * sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]);
      q.w = (m[0][2] - m[2][0]) / s;
      q.x = (m[0][1] + m[1][0]) / s;
      q.y = 0.25f * s;
      q.z = (m[1][2] + m[2][1]) / s;
    } else {
      float s = 2.0f * sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]);
      q.w = (m[1][0] - m[0][1]) / s;
      q.x = (m[0][2] + m[2][0]) / s;
      q.y = (m[1][2] + m[2][1]) / s;
      q.z = 0.25f * s;
    }
  }

  return q;
}

Color getTextColor(Color backgroundColor)
{
  float luminosity = (backgroundColor.r * 0.299f + backgroundColor.g * 0.587f + backgroundColor.b * 0.114f) / 255.0f;
  return luminosity > 0.5f ? BLACK : WHITE;
}

void drawButton(Vector2 pos, float width, float height, HidNpadButton button,
                const char *label, float roundness, float fontOffsetX,
                float fontOffsetY, float fontSize, int materialIndex,
                int materialIndexJoyL, int materialIndexJoyR) {
  Rectangle rec;
  rec.x = pos.x - width;
  rec.y = pos.y - height;
  rec.height = height * 2;
  rec.width = width * 2;
  DrawRectangleRounded(rec, roundness, 8,
                       getPacketData()->keys & button
                           ? *(Color *)&cfg.colActive
                           : buttonColor);
  pos.x += fontOffsetX;
  pos.y += fontOffsetY;
  DrawTextEx(buttonFont, label, pos, fontSize, 4, cfg.useSystemButtonColor
              ? getTextColor(buttonColor)
              : *(Color *)&cfg.colFont);
  if (materialIndex != -1) {
    _controllerModel.materials[materialIndex].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : buttonColor;
  }
  if (materialIndexJoyL != -1) {
    _joyLeftModel.materials[materialIndexJoyL].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : buttonColor;
  }
  if (materialIndexJoyR != -1) {
    _joyRightModel.materials[materialIndexJoyR].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : buttonColor;
  }
}

float clamp(float num, float min, float max)
{
  if (num < min)
    num = min;
  else if (num > max)
    num = max;
  return num;
}

Color generateStickColor(Color buttonColor)
{
  float luminosity = (buttonColor.r * 0.299f + buttonColor.g * 0.587f + buttonColor.b * 0.114f) / 255.0f;
  int adjustFactor = luminosity > 0.5f ? -65 : 65;

  Color myStickColor;
  myStickColor.r = (unsigned char)clamp(buttonColor.r + adjustFactor, 0, 255);
  myStickColor.g = (unsigned char)clamp(buttonColor.g + adjustFactor, 0, 255);
  myStickColor.b = (unsigned char)clamp(buttonColor.b + adjustFactor, 0, 255);
  myStickColor.a = 255;
  return myStickColor;
}

int main() {
  cfg = loadConfig();
  InitWindow(SWIDTH, SHEIGHT, "HOS-InputDisplay");
  //SetConfigFlags(FLAG_MSAA_4X_HINT);

  buttonFont = LoadFontEx(cfg.fontPath, 38, 0, 250); // TODO: try 38, null, 0 ?
  fatFont = LoadFontEx(cfg.fatFontPath, 38, 0, 250);

  Camera3D camera = {0};
  camera.projection = CAMERA_PERSPECTIVE;
  camera.fovy = 45;
  camera.target.x = 0;
  camera.target.y = 0;
  camera.target.z = -6;
  camera.up.x = 0;
  camera.up.y = 1;
  camera.up.z = 0;
  camera.position.x = 0;
  camera.position.y = -15;
  camera.position.z = 15;

  _controllerModel = LoadModel("Res/Controller.glb");
  _joyLeftModel = LoadModel("Res/JoyL.glb");
  _joyRightModel = LoadModel("Res/JoyR.glb");

  Vector3 controllerModelPos;
  controllerModelPos.x = 0;
  controllerModelPos.y = 0;
  controllerModelPos.z = 0;

  Quaternion slerpTo;
  slerpTo.x = 0;
  slerpTo.y = 0;
  slerpTo.z = 0;
  slerpTo.w = 1;
  Quaternion baseRotation;
  baseRotation.x = 0;
  baseRotation.y = 0;
  baseRotation.z = 0;
  baseRotation.w = 1;

  Quaternion slerpTo2;
  slerpTo.x = 0;
  slerpTo.y = 0;
  slerpTo.z = 0;
  slerpTo.w = 1;
  Quaternion baseRotation2;
  baseRotation.x = 0;
  baseRotation.y = 0;
  baseRotation.z = 0;
  baseRotation.w = 1;

  int cooldown = -1; // calibrationCooldown
  /* FINAL */float initialCooldown = cfg.packetsPerSecond / 2;

  controllerColorLeft.r = cfg.colController.r;
  controllerColorLeft.g = cfg.colController.g;
  controllerColorLeft.b = cfg.colController.b;
  controllerColorLeft.a = cfg.colController.a;
  controllerColorRight.r = cfg.colController.r;
  controllerColorRight.g = cfg.colController.g;
  controllerColorRight.b = cfg.colController.b;
  controllerColorRight.a = cfg.colController.a;
  buttonColor.r = cfg.colInactive.r;
  buttonColor.g = cfg.colInactive.g;
  buttonColor.b = cfg.colInactive.b;
  buttonColor.a = cfg.colInactive.a;
  stickColor = generateStickColor(buttonColor);

  initSocketShit(cfg);
  updateSocketShit();

  // bugfix: recalibrate gyro on first frame
  // (because uncalibrated values can be buggy)
  slerpTo = toQuaternion(getPacketData()->states[0].direction.direction);
  slerpTo.x *= -1;
  slerpTo.y *= -1;
  slerpTo.z *= -1;
  slerpTo2 = toQuaternion(getPacketData()->states[1].direction.direction);
  slerpTo2.x *= -1;
  slerpTo2.y *= -1;
  slerpTo2.z *= -1;
  cooldown = initialCooldown;

  while (1) {
    updateSocketShit();

    if (cfg.useSystemControllerColor)
    {
      controllerColorLeft.r = (unsigned char)(getPacketData()->colors[0].shellColor & 0xff);
      controllerColorLeft.g = (unsigned char)((getPacketData()->colors[0].shellColor & 0xff00) >> 8);
      controllerColorLeft.b = (unsigned char)((getPacketData()->colors[0].shellColor & 0xff0000) >> 0x10);
      controllerColorLeft.a = 255;

      controllerColorRight.r = (unsigned char)(getPacketData()->colors[1].shellColor & 0xff);
      controllerColorRight.g = (unsigned char)((getPacketData()->colors[1].shellColor & 0xff00) >> 8);
      controllerColorRight.b = (unsigned char)((getPacketData()->colors[1].shellColor & 0xff0000) >> 0x10);
      controllerColorRight.a = 255;
    }
  
    if (cfg.useSystemButtonColor)
    {
      buttonColor.r = (unsigned char)(getPacketData()->colors[0].buttonColor & 0xff);
      buttonColor.g = (unsigned char)((getPacketData()->colors[0].buttonColor & 0xff00) >> 8);
      buttonColor.b = (unsigned char)((getPacketData()->colors[0].buttonColor & 0xff0000) >> 0x10);
      buttonColor.a = 255;
      stickColor = generateStickColor(buttonColor);
    }


    BeginDrawing();
    ClearBackground(*(Color *)&cfg.colBg);

    if (cfg.enableGyroModels)
    {
      // new behavior: recalibrate only upon initial press.
      // mimics Splatoon 3.
      if (getPacketData()->keys & gyroCalibrateButton) {
        if (gyroCalibrateButtonHeld) {
          // button is being held; do nothing
        } else {
          // note: the *real* position is recalibrated instantly.
          // the rotation easing animation is just a facade.
          slerpTo = toQuaternion(getPacketData()->states[0].direction.direction);
          slerpTo.x *= -1;
          slerpTo.y *= -1;
          slerpTo.z *= -1;
          slerpTo2 = toQuaternion(getPacketData()->states[1].direction.direction);
          slerpTo2.x *= -1;
          slerpTo2.y *= -1;
          slerpTo2.z *= -1;
          cooldown = initialCooldown;
          gyroCalibrateButtonHeld = true;
        }
      } else {
        // button is unpressed
        gyroCalibrateButtonHeld = false;
      }

      // handle recalibration rotation easing
      if (cooldown > 0) {
        cooldown--;

        // rounding errors inbound.
        // but it works out fine, and thus doesn't matter.
        // it just means acceleration may be a little wonky.
        baseRotation =
            QuaternionSlerp(baseRotation, slerpTo,
                            (initialCooldown - (float)cooldown) / initialCooldown);
        baseRotation2 =
            QuaternionSlerp(baseRotation2, slerpTo2,
                            (initialCooldown - (float)cooldown) / initialCooldown);
      }

      if ((getPacketData()->styleTag & (int)(HidNpadStyleTag_JoyDual)) != 0)
      {
        _joyLeftModel.materials[1].maps[0].color = controllerColorLeft;
        _joyLeftModel.materials[5].maps[0].color = controllerColorLeft;
        _joyLeftModel.materials[6].maps[0].color = controllerColorLeft;

        _joyLeftModel.materials[2].maps[0].color = buttonColor;
        _joyLeftModel.materials[3].maps[0].color = buttonColor;
        _joyLeftModel.materials[4].maps[0].color = buttonColor;

        _joyRightModel.materials[7].maps[0].color = controllerColorRight;
        _joyRightModel.materials[8].maps[0].color = controllerColorRight;
        _joyRightModel.materials[9].maps[0].color = controllerColorRight;

        _joyRightModel.materials[1].maps[0].color = buttonColor;

        Quaternion leftQuat = toQuaternion(getPacketData()->states[0].direction.direction);
        leftQuat.x *= -1;
        leftQuat.y *= -1;
        leftQuat.z *= -1;
        _joyLeftModel.transform = QuaternionToMatrix(
          QuaternionMultiply(QuaternionInvert(baseRotation), leftQuat));
        
        Quaternion rightQuat = toQuaternion(getPacketData()->states[1].direction.direction);
        rightQuat.x *= -1;
        rightQuat.y *= -1;
        rightQuat.z *= -1;
        _joyRightModel.transform = QuaternionToMatrix(
          QuaternionMultiply(QuaternionInvert(baseRotation2), rightQuat));
        
        BeginMode3D(camera);
        Vector3 posL;
        posL.x = -4;
        posL.y = 0;
        posL.z = 0;
        DrawModel(_joyLeftModel, posL, 2, WHITE);
        Vector3 posR;
        posR.x = 4;
        posR.y = 0;
        posR.z = 0;
        DrawModel(_joyRightModel, posR, 2, WHITE);
        EndMode3D();
      }
      else if ((getPacketData()->styleTag & (int)(HidNpadStyleTag_FullKey)) != 0)
      {
        _controllerModel.materials[2].maps[0].color = controllerColorLeft;
        _controllerModel.materials[7].maps[0].color = buttonColor;

        Quaternion quat = toQuaternion(getPacketData()->states[0].direction.direction);
        quat.x *= -1;
        quat.y *= -1;
        quat.z *= -1;
        _controllerModel.transform = QuaternionToMatrix(
            QuaternionMultiply(QuaternionInvert(baseRotation), quat));

        BeginMode3D(camera);
        DrawModel(_controllerModel, controllerModelPos, 2, WHITE);
        EndMode3D();
      }
    }

    Vector2 pos;
    pos.x = 100;
    pos.y = 180;
    drawStick(pos, getPacketData()->lPos, HidNpadButton_StickL, 8, 7, -1);
    pos.x = 380;
    pos.y = 270;
    drawStick(pos, getPacketData()->rPos, HidNpadButton_StickR, 9, -1, 10);
    pos.x = 550;
    pos.y = 180;
    drawButton(pos, 25, 25, HidNpadButton_A, "A", 1, -12, -19, 38, 10, -1, 3);
    pos.x = 450;
    pos.y = 180;
    drawButton(pos, 25, 25, HidNpadButton_Y, "Y", 1, -12, -19, 38, 13, -1, 4);
    pos.x = 500;
    pos.y = 230;
    drawButton(pos, 25, 25, HidNpadButton_B, "B", 1, -12, -19, 38, 12, -1, 2);
    pos.x = 500;
    pos.y = 130;
    drawButton(pos, 25, 25, HidNpadButton_X, "X", 1, -12, -19, 38, 11, -1, 5);
    pos.x = 324;
    pos.y = 220;
    drawButton(pos, 12, 12, HidNpadButton_Plus, "+", 1, -9, -19, 38, 14, -1, 12);
    pos.x = 276;
    pos.y = 220;
    drawButton(pos, 12, 12, HidNpadButton_Minus, "-", 1, -7, -21, 38, 15, 3, -1);
    pos.x = 500;
    pos.y = 70;
    drawButton(pos, 40, 15, HidNpadButton_R, "R", 1, -12, -20, 38, 18, -1, 6);
    pos.x = 500;
    pos.y = 30;
    drawButton(pos, 40, 15, HidNpadButton_ZR, "ZR", 1, -22, -19, 38, 16, -1, 11);
    pos.x = 100;
    pos.y = 70;
    drawButton(pos, 40, 15, HidNpadButton_L, "L", 1, -12, -20, 38, 17, 12, -1);
    pos.x = 100;
    pos.y = 30;
    drawButton(pos, 40, 15, HidNpadButton_ZL, "ZL", 1, -22, -19, 38, 1, 11, -1);
    pos.x = 220;
    pos.y = 240;
    drawButton(pos, 15, 15, HidNpadButton_Up, "", .5, 0, 0, 38, 3, 8, -1);
    pos.x = 220;
    pos.y = 300;
    drawButton(pos, 15, 15, HidNpadButton_Down, "", .5, 0, 0, 38, 5, 10, -1);
    pos.x = 190;
    pos.y = 270;
    drawButton(pos, 15, 15, HidNpadButton_Left, "", .5, 0, 0, 38, 6, 4, -1);
    pos.x = 250;
    pos.y = 270;
    drawButton(pos, 15, 15, HidNpadButton_Right, "", .5, 0, 0, 38, 4, 9, -1);

    pos.x = 220;
    pos.y = 270;
    Rectangle rec;
    rec.x = pos.x - 20; // 200
    rec.y = pos.y - 15; // 255
    rec.height = 15 * 2;
    rec.width = 20 * 2;
    DrawRectangleRounded(rec, 0, 8, buttonColor);
    rec.x = pos.x - 15; // 205
    rec.y = pos.y - 20; // 250
    rec.height = 20 * 2;
    rec.width = 15 * 2;
    DrawRectangleRounded(rec, 0, 8, buttonColor);

    if (cooldown > 0) {
      pos.x = 180;
      pos.y = 10;
      DrawTextEx(fatFont, "Recalibrating", pos, 38, 2, WHITE);
    }

    EndDrawing();

    global++;
  }

  free((void *)cfg.host);
  free((void *)cfg.fontPath);
  return 0;
}